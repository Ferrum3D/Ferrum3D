# Stage 3: bounded surface evaluation

Stage 3 runs the first loose-tiling pass and evaluates all sample materials into four `R32_UINT` GBuffer planes. Forward shading remains the default. Use `--visibility-debug` with `evaluation-tile`, `evaluation-group`, `surface-color`, `surface-normal`, `surface-properties`, `surface-emissive`, or `surface-route` in either sample. Existing Stage 2 diagnostics remain selectable.

## Commands and scratch

`SurfaceWorkBuilder` processes consecutive 128×128 tiles in bounded windows. A stable eight-pass, four-bit GPU radix sort groups pixels by mesh/material group within each tile. Group runs produce paired 16-bit local pixel coordinates, one writer per 32-bit word. Each run starts at an even pixel offset, and tail commands store a valid count without padding the pixel list to a complete wave. The worst-case paired storage is four bytes per source pixel, accounting for one-pixel runs and alignment gaps.

A second stable radix sort orders sparse run references by evaluation pipeline, preserving tile/group order. Parallel block scans and a block-total scan assign wave offsets. GPU-generated shader ranges and three-word indirect arguments drive one dispatch per registered evaluation pipeline per window; empty ranges dispatch zero groups. Large ranges use X up to 65,535 and additional Y groups, with a range check in the consumer. No material-visibility readback or fixed material-count cap is used. The 16 radix bins represent key digits, not a limit of 16 materials.

Logical evaluation commands use 32 or 64 lanes. The scene selects the supported preferred count and the runtime compiles the matching shader variant. Exact lane-count control is requested when available for the 64-lane variant. The workgroup is uniform in geometry/material group; transforms and instance parameters remain lane-specific. Correctness does not depend on hardware lane numbering, implicit pixel quads, or wave64 assumptions.

For `N` source pixels and `S` evaluation slots, logical buffer payloads total `64N + 136(N/256) + 20 max(S,1)` bytes. The budget conservatively charges `72N + 136(N/256) + 40 max(S,1) + 4096`, accounting for run-buffer and shader-array allocation rounding. Window capacity is a power of two and capped at 64 tiles. The default 16 MiB budget normally selects eight tiles (`N=131,072`), with an upper charge near 9.1 MiB; no pathological full-screen wave array is allocated. The buffer set is reused only after every evaluation dispatch for the current window. Peak image storage additionally includes 16 bytes/pixel for the four planes.

This is a correctness-oriented radix baseline. Each window has 54 construction dispatches plus the registered evaluation dispatches. Small shared-memory histograms or hashing can be measured and added later without losing this high-diversity path.

## Material and surface contracts

Textured evaluates albedo with Tint and per-instance UV scale. BunnyVariants preserves both authored palettes and needs no parameter allocation, UV or tangent fetch. StandardPBR samples base color, linear ORM, BC5 normals and emissive with `SampleGrad`, preserving transformed tangent handedness and the existing +Y normal convention. UV scale multiplies both UVs and gradients. All source samples are linear after their existing texture views; bounded base colors receive explicit sRGB8 encoding only when stored.

G0 contains encoded RGB8 and a dynamically assigned shading route. G1 contains signed 16-bit octahedral coordinates, including the negative-Z fold. G2 contains roughness, metallic, AO and flags; SimpleLit reserves the property bytes as zero. G3 packs nonnegative scene-linear emissive as R11/G11/B10. The extended-valid flag controls all G3 writes/reads. Emissive is multiplied by 1,000 cd/m² before packing, with pre-exposure deferred to shading. Packing sanitizes invalid emissive and clamps to the finite channel ranges (65,024 for R/G, 64,512 for B).

Only G0 is cleared globally. Covered supported pixels write G0/G1/G2 exactly once. Missing evaluation/shading bindings, or shading slots beyond the compact 254-route capacity, produce route 255 and magenta diagnostics. Background is route zero and must not read other planes. Evaluation and shading identities come from separate shader/define bindings, without a predefined shading-model enum. All bunny evaluation variants and Textured share the SimpleLit shading binding; StandardPBR has its own binding.

Standalone SimpleLit/PBR decoder kernels provide those concrete shading bindings and preserve the existing equations. They are not connected to rendering in Stage 3. Stage 4 must add the independent second loose-tiling classification and command-driven shading submission before deferred lighting can replace forward shading.

## Synchronization and verification

The DB upload loop is dirty-page-only again. Clean pages retain their readable state from prior frames. Scratch read inputs use SRV declarations, while writes use UAV declarations; indirect arguments use the existing compute-write → indirect-read dependency. All work stays on the current graphics queue. Visibility, geometry and parameter snapshots keep the existing single-resident-LOD frame contract; asynchronous scheduling and final reload/stream-out stress remain later checkpoints.

Validated on 2026-10-14: the seven graphics tests (including the extended GPU probes), all 24 GameFramework tests and the standalone reflection-link check pass. Required targets build, and the three sample material depots were manually recooked. RendererSample passes forward, surface color, evaluation group and emissive smoke runs. GameSample passes forward, surface normal, properties and route smoke runs. No GPU validation errors were reported in the completed runs; existing validation configuration/unused-output warnings remain.

One concurrent GameSample forward smoke invocation stopped with a breakpoint before its readiness message, without recording an assertion or validation error. A debugger run and subsequent serial smoke runs passed; the cause was not reproduced. This remains a validation limitation rather than a claimed resolved bug.

Automated probes cover 257 groups across partial 257×129 images, multiple windows, 32/64-lane commands, empty shader ranges, exactly-once visits, pair packing and group/pipeline routing. Codec probes cover sRGB quantization, negative-Z octahedral normals, emissive and flag packing. The raster probe also dispatches a real palette evaluator from GPU-built arguments and reads its GBuffer output. An empty-view iteration after populated iterations checks scratch reuse.

Manual checkpoint: inspect every plane, material boundaries, palette/Tint colors, UV scale, normal-map orientation, emissive and streaming changes in both samples. Compare mip selection during camera motion, examine the construction/evaluation sequence in a GPU capture, and measure scratch allocations and stage timings. Automated checks do not replace these visual captures or performance measurements.
