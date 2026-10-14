# Surface classification performance redesign

The recommended dense full-view replacement is implemented, replacing the cooperative radix path. This document preserves the research and tradeoffs that motivated it; the current implementation uses persistent scratch and 32-bit coordinate words. The user's updated constraints permit full-view scratch and explicit shader/group limits; the original requirement to retain bounded eight-tile windows is no longer a design priority.

## Findings in the former code and capture

`SurfaceWorkBuilder` reuses one record/pixel/command allocation across eight-tile windows. Every window therefore must finish classification, scans, command generation and material evaluation before the next overwrites those buffers. The loop exists to satisfy the original worst-case scratch budget, not to provide a performance benefit. It creates genuine write-after-read and read-after-write dependencies; removing barriers without changing allocation/lifetimes would be incorrect.

The former cooperative tile sort launches only eight 256-thread workgroups for a normal window. Each lane processes 64 consecutive records, so adjacent lanes initially load addresses 512 bytes apart. Eight radix rounds repeatedly read and rewrite global ping-pong buffers. In each round only 16 lanes perform the 256-chunk offset calculation while the rest wait. The capture's 5.5% compute occupancy, substantial long-scoreboard stalls and approximately 0.55 ms sort are consistent with this structure. The screenshot does not establish register spills or actual memory placement; those need separate inspection. High PCIe utilization alone does not establish that surface buffers reside in host memory.

Increasing scratch and submitting all tiles together removes the window dependencies and improves available parallelism, but does not eliminate this inefficient global-memory sort. Renaming/grouping FrameGraph passes or moving the same serial sequence inside one callback would not fix the GPU workload.

## What the presentations actually do

[James McLaren, Adventures with Deferred Texturing in Horizon Forbidden West](../presentations/AdventuresWithDeferred_McLaren_James.pdf): PDF pages 31-36 describe 128x128 loose tiles, material-uniform wave commands and one indirect dispatch per shader across the screen. Pages 53-60 describe masks, popcounts, counts, prefix sums and output scatter. The presentation does not prescribe an eight-round segmented radix sort. Bonus slide 108 is PDF page 109: each pass supports 32 batch groups, with common shader, textures/constants and geometry; instances may differ. This is a group-binding limit within a shader pass, not a limit of 32 shaders per screen tile. Masks are used to allocate counters only for group/tile combinations that occur. The notes explain why a dense 32-counter array for every tile of 256 passes at 4K would cost roughly 16 MB.

[Dominik Lazarek and Philip Hammer, Visibility Buffer and Deferred Rendering in DOOM: The Dark Ages](../presentations/Lazarek-Hammer-visibility-buffer-and-deferred-rendering-in-doom-the-dark-ages.pdf): pages 19-21 describe 256x256 tiles, wave-aligned pixel commands, counts and prefix sums ordered by shader/tile/material, restricting bookkeeping to the visible set registered during gathering/culling. Page 21 discusses dispatch grouping and power-of-two buckets when emulating multi-indirect dispatch outside D3D12. Pages 27-28 discuss divergent mesh/entity accesses and separate uniform/divergent permutations. Page 55 explicitly accepts substantial resolution-dependent intermediate storage. Its separate interpolation buffers address material-shader register pressure, rather than classification sorting, so adding those would not directly fix this bottleneck.

The useful common principle is counting into known buckets and prefix-scattering commands across the whole view. Strict ascending persistent group-row order is not required for correctness; deterministic bucket order can retain shader/tile locality without sorting pixel keys. Pixel order inside each group can be arbitrary while keeping group bindings uniform per wave.

## Recommended first replacement: dense full-view count/prefix/scatter

1. Build a compact, frame-stable table of resident renderable group bindings, ordered by evaluation shader. Map persistent `MeshGroupTable` row IDs to dense binding IDs. Bindings retain geometry, material parameters and independent evaluation/shading routes. Do not use the sparse database row high-water mark as the dense group count or collapse distinct geometry bindings merely because their shaders match.
2. Allocate full-view pixel commands, wave commands and counters. Clear counters and G0 in one preparation phase. Size from the view dimensions and actual dense group count, with frames-in-flight retention and safe resize. Keep allocations persistent/reusable when dimensions/capacity permit.
3. Classify using many small screen workgroups (for example 16x16 threads, 64 workgroups per 128x128 tile), counting `(shader, tile, binding)` buckets. Aggregate matching group IDs within a wave before issuing global atomics. Counting every pixel with one contended atomic, especially a full-screen single-material scene, is not an acceptable final fast path.
4. Prefix-scan bucket counts into pixel offsets and rounded wave counts, then scan wave counts into wave offsets. These can be paired integer scans. Lay out buckets by shader, then tile, then binding, so contiguous shader ranges and tile locality follow from offsets. Use hierarchical parallel scans, not a serial workgroup walking all full-view pixels. Reset scatter cursors alongside offset generation where safe.
5. Reread visibility and scatter local coordinates into the bucket ranges using wave-aggregated cursor reservations. A reservation returns a disjoint range; active lane prefix ranks assign destinations. Native independent 16-bit stores require the corresponding Vulkan storage feature and shader compiler support, neither currently enabled in the device setup. Start with one 32-bit coordinate word per pixel to avoid shared-word races; alternatively use 32-bit staging plus a later pair-packing pass. Atomic halfword OR is possible only with explicit zero initialization and measured contention, and is not the default choice.
6. Generate wave commands and shader ranges/indirect arguments from counts and offsets. These outputs need the scans, not the completed coordinate scatter; the two output phases can be scheduled without introducing a false dependency between their distinct destinations. Evaluation waits for both.
7. Submit one indirect material evaluation per registered evaluation PSO for the whole view. Empty ranges remain zero. Retain the existing large-range X/Y linearization and command valid-count guards. Missing bindings remain explicit diagnostic pixels.

The implementation uses five preparation dispatches when buckets fit one scan block; larger tables add two dispatches per extra scan level. One evaluation dispatch per shader and the debug display follow. Initial or changed routing adds three buffer uploads. These are submission counts, not GPU-time measurements. Dispatch count must not grow with the number of eight-tile windows. Classification should expose thousands of workgroups at sample resolution instead of eight long workgroups at a time.

The two classification reads are a deliberate bandwidth trade for eliminating eight global sort rounds. Shader/group masks or precomputed per-view routing reduce metadata lookups; avoid adding a large per-pixel identity record unless measurement shows those lookups dominate. Bindings remain shader-derived and can later incorporate each pass's parameter equivalence separately.

## Capacity proof and example memory

Let `T` be the number of 128x128 tiles, `P = 16384 T` their padded pixel capacity, `G` the dense resident binding count, `B = T G` the dense bucket count, `L` the logical lane count and `R` the number of occupied buckets. Then `R <= min(P,B)` and wave capacity is conservatively `min(P, ceil(P/L) + min(P,B))`. This follows from summing `ceil(bucketPixels/L)` over occupied buckets. Thus we can prove sufficient command capacity without allocating a 16-byte command for every pixel in ordinary scenes.

A straightforward starting budget is `4P` bytes of coordinate words, 16 bytes per wave and approximately `20B` bytes of counters/offsets/cursors, plus scan tables, shader ranges and binding maps. The examples use 32 lanes and the conservative occupied-bucket bound. They exclude GBuffer/depth/visibility images, allocation alignment and multiple retained resource generations.

| View | Resident bindings | Dense command/counter estimate |
| --- | ---: | ---: |
| 1280x720 | 1 | 4.22 MiB |
| 1280x720 | 32 | 4.28 MiB |
| 1280x720 | 257 | 4.75 MiB |
| 1920x1080 | 257 | 10.68 MiB |
| 3840x2160 | 32 | 36.42 MiB |
| 3840x2160 | 257 | 40.36 MiB |

Actual wave usage is usually smaller. When `G` is very large, dense `T G` bookkeeping and the occupied-bucket command bound can dominate; this is where sparse masks are justified. Any imposed resident-group limit must be checked before indexing, with explicit fallback/diagnostics rather than silent omission. For normal scenes this approach requires no fixed shader-count limit and supports the existing 257-group test. A user-acceptable cap could be added to bound metadata, but is not necessary just to remove sorting.

## Alternatives and tradeoffs

| Approach | Benefit | Cost/limitation | Assessment |
| --- | --- | --- | --- |
| Full-view current radix sort | Small implementation change; removes window reuse chain; more tile workgroups | About 72 bytes per padded pixel of conservative scratch, still eight rounds and strided accesses; remove the current 64-tile capacity cap | Useful comparison/control, not the preferred endpoint |
| Dense full-view histogram | Removes sort; straightforward existing group identity; predictable offsets and correctness | `T G` metadata, two visibility reads and atomics; must aggregate reservations | Recommended first replacement |
| Horizon-style mask compaction | Counters only for occupied combinations; avoids dense metadata at large scene group counts | Extra mask/compaction phases; 32 bindings per logical classification bucket and mapping maintenance | Recommended scaling path after dense profiling |
| Local tile hash table, e.g. 64/128 bindings | No dense table across all resident groups; cheap for ordinary low-diversity tiles | Collisions, probing, contention and measurable occupancy pressure; overflow needs a GPU fallback | Possible alternative, not the first baseline |
| Per-shader full-screen evaluator | Very few passes, straightforward shader filtering | Repeated visibility/geometry work per shader and empty lanes; potentially divergent materials | Benchmark control for the single-shader sample, not a general solution |
| Larger 256x256 tiles | Fewer tile/group counters and partially filled wave tails, as in DOOM | Coarser locality, more work per tile; more expensive if one workgroup owns an entire tile | Evaluate after changing the classifier; not a substitute for it |

For sparse masks, divide each shader's resident bindings into logical 32-binding buckets on the CPU, preserving the actual shader identity. Overflow can create another logical bucket for the same shader rather than dropping materials. Clear per-tile masks, generate used bits, popcount/prefix to compact counters, count/scatter pixels, then concatenate all logical buckets into the real shader's command range. Logical buckets are data organization; they must not become a separate FrameGraph pass per bucket/tile. This adapts Horizon's practical restriction without hard-coding a shading-model enum or losing current material coverage.

A tile-local hash cap means a limit on distinct group bindings, not merely distinct shaders: a tile may use one shader with hundreds of geometry/material groups. On overflow, mark the tile and rebuild only those tiles with a separate general GPU path. Do not emit partial fast-path work and full fallback work for the same pixel. A divergent shader-only fallback is also possible but must replace uniform-binding assumptions, not simply substitute a different group key.

## Validation and profiling before claiming a speedup

- Preserve every covered pixel exactly once, background and unsupported diagnostics, partial tiles, independent routes, 32/64 lanes, empty ranges and large indirect counts. Add dense-ID holes/reloads, multiple bindings sharing a shader, odd group sizes and deliberately exceeded fast-path limits.
- Capture at the same view/resolution with validation disabled. Record classification, scans, scatter, evaluation and total GPU time separately, as well as CPU graph construction/recording. Include one full-screen binding, many small bindings, one shader with many groups and many shaders.
- Inspect actual memory heap flags/budgets and retained generations if PCIe traffic persists. VMA's device preference is not proof of physical placement, especially under budget pressure. Track persistent allocation reuse rather than only logical buffer sizes.
- Verify dispatch/barrier counts are constant in tile count except for necessary scan hierarchy, and buffers no longer cycle through build/evaluate/overwrite for eight-tile windows. Target a small fraction of the reported roughly 6 ms classification cost; no concrete time can be promised before measuring the replacement.
