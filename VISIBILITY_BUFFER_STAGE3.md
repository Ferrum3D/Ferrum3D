# Stage 3: bounded surface evaluation

Stage 3 runs the first loose-tiling pass and evaluates all sample materials into four `R32_UINT` GBuffer planes. Forward shading remains the default. Use `--visibility-debug` with `evaluation-tile`, `evaluation-group`, `surface-color`, `surface-normal`, `surface-properties`, `surface-emissive`, or `surface-route` in either sample. Existing Stage 2 diagnostics remain selectable.

## Commands and scratch

`SurfaceWorkBuilder` classifies the whole view using dense histogram buckets ordered by evaluation shader, 128x128 tile and group binding. CPU routing maps persistent group rows to compact binding IDs; holes in the GPU database are not counted as active groups. Distinct geometry/material bindings stay separate even when they share a shader. Missing evaluation or shading routes produce the existing magenta diagnostic.

Clear resets bucket counters, scatter cursors and G0. Many 256-thread screen workgroups count pixels, using wave-aggregated atomics for matching buckets. Paired hierarchical prefix scans assign pixel offsets and wave offsets. A second visibility traversal scatters coordinates using disjoint wave-aggregated reservations. Each coordinate occupies one 32-bit word, with `localX | (localY << 7)` in the low bits; this avoids racing halfword writes without requiring native 16-bit storage. Pixel order inside a binding is arbitrary. Shader/tile/binding command order is deterministic, and valid-count tails preserve partial tiles and partially filled waves.

Parallel command generation uses the completed offsets to emit 16-byte wave commands, shader ranges and three-word indirect arguments. One indirect dispatch per evaluation shader consumes all view commands. Empty ranges dispatch zero. Large ranges retain the X<=65535/Y linearization and consumer range guards. There is no radix sort or serial tile-window build/evaluate/overwrite cycle.

Logical evaluation commands use 32 or 64 lanes. Count/scatter wave aggregation works with the hardware wave width independently of the evaluation command width. Instance transforms and instance parameters remain lane-specific within group-uniform commands.

For `T` tiles, padded capacity `P=16384T`, dense bindings `G`, buckets `B=TG` and logical lanes `L`, command capacity is conservatively `min(P, ceil(P/L)+min(P,B))`. Each occupied bucket adds at most one partial wave. Pixel capacity is `P` words, counters/cursors are `8B` bytes, root paired offsets are `8B` bytes, and scan levels add progressively smaller paired arrays. Lookup, binding and shader tables add small routing storage. All arithmetic is checked against the 32-bit buffer-address limits; no fixed shader/group cap is imposed. Memory scales with resolution and resident bindings rather than a 16 MiB window budget.

The renderer retains this buffer set and skips unchanged routing uploads. Resolution, binding, route count or lane changes replace it; resource-pool fences retire old generations safely. Sequential views use the same graphics queue and explicit resource dependencies, matching the renderer's shared-target policy. GBuffer images still use the existing per-view transient allocation path.

Stable GameSample frames with one binding at 1280x720 use five construction dispatches, one material dispatch and one debug dispatch: seven surface dispatches, down from 442 originally and 50 in the cooperative radix intermediate. Initial/reconfigured frames additionally upload three routing buffers. Views with more than 256 buckets require additional scan/propagation dispatches; the number grows with scan hierarchy rather than tile windows. Concrete GPU-time improvement still requires a new capture.

## Material and surface contracts

Textured evaluates albedo with Tint and per-instance UV scale. BunnyVariants preserves both authored palettes and needs no parameter allocation, UV or tangent fetch. StandardPBR samples base color, linear ORM, BC5 normals and emissive with `SampleGrad`, preserving transformed tangent handedness and the existing +Y normal convention. UV scale multiplies both UVs and gradients. All source samples are linear after their existing texture views; bounded base colors receive explicit sRGB8 encoding only when stored.

G0 contains encoded RGB8 and a dynamically assigned shading route. G1 contains signed 16-bit octahedral coordinates, including the negative-Z fold. G2 contains roughness, metallic, AO and flags; SimpleLit reserves the property bytes as zero. G3 packs nonnegative scene-linear emissive as R11/G11/B10. The extended-valid flag controls all G3 writes/reads. Emissive is multiplied by 1,000 cd/mР’Р† before packing, with pre-exposure deferred to shading. Packing sanitizes invalid emissive and clamps to the finite channel ranges (65,024 for R/G, 64,512 for B).

Only G0 is cleared globally. Covered supported pixels write G0/G1/G2 exactly once. Missing evaluation/shading bindings, or shading slots beyond the compact 254-route capacity, produce route 255 and magenta diagnostics. Background is route zero and must not read other planes. Evaluation and shading identities come from separate shader/define bindings, without a predefined shading-model enum. All bunny evaluation variants and Textured share the SimpleLit shading binding; StandardPBR has its own binding.

Standalone SimpleLit/PBR decoder kernels provide those concrete shading bindings and preserve the existing equations. Stage 4 connects them through independent G0 classification and indirect shading commands in the selectable deferred mode; see `VISIBILITY_BUFFER_STAGE4.md`.

## Synchronization and verification

The database upload loop remains dirty-page-only. Clean pages retain readable state. Persistent scratch is explicitly committed as device-local; each pass declares its reads and writes. Pixel scatter and command emission write distinct destinations and share only scan/routing reads. Evaluation consumes both, and indirect arguments retain compute-write to indirect-read synchronization. All work runs on the current graphics queue; existing geometry/material residency and frame snapshot retention remain in place.

The surface probe covers 257 bindings with nonzero persistent row IDs and holes, 257x129 partial tiles, 32/64 evaluation lanes, one shader, five/17 shader slots including interior empty ranges, routing changes and a reused populated allocation followed by an empty view. The real raster probe still evaluates the mirrored/nonuniform transformed triangle and checks GBuffer color, normal and background. Tests validate exactly-once visits, correct group/shader, local-coordinate reserved bits and zero empty dispatches.

Validated on 2026-10-14: Debug graphics tests pass all seven cases with API-only and GPU-assisted validation, with no validation errors. Debug GameSample surface-normal and RendererSample surface-color smoke runs pass, including API validation. Configure, reflection generation, sample/test builds and manual material-depot builds complete. This verifies correctness and submission structure, not steady-state GPU timing.

Validation is disabled by default. `--api-validation` enables Vulkan checks; `--gpu-validation` additionally enables GPU instrumentation and implies API validation. Use neither flag for timing captures. The earlier validation-only investigation and short smoke runs did not establish steady-state GPU performance.

Manual checkpoints remain material boundaries, normal-map orientation, palette/tint colors, UV scale, emissive and streaming/reload/resize behavior. Capture classification, scans, scatter, evaluation, CPU graph recording and retained memory generations at the same resolution/view before assigning a frame-time improvement. See [the redesign analysis](VISIBILITY_BUFFER_CLASSIFICATION_OPTIMIZATION.md) for sources, alternatives and capacity tradeoffs.
