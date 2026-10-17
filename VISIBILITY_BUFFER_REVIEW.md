# Visibility Buffer review

Reviewed on 2026-10-17 against `9697f546684a9e55a880be5ae5c284afa58f000a`.

The review below describes that baseline. The implementation follow-up at the end records the applied changes.

## Scope and result

The range is inclusive of `9f76cf2fbbcadd2c89e7f956f936ee96ccb1b21f` (Visibility Buffer Stage 1). The comparison base is its parent, `45d6743dddebc9220b8b1c0830842e160dede2b1`.

| Commit | Description |
| --- | --- |
| `9f76cf2f` | Visibility Buffer Stage 1 |
| `3fb4d95e` | Implement visibility buffer stage 2 |
| `e130069d` | Implement visibility buffer stage 3 |
| `daf4e86f` | Optimizations |
| `e7a3ac26` | Stage 4 |
| `bd5f22ff` | Stage 5 |
| `b424808a` | Cleanup |
| `9697f546` | Cleanup |

The review follows the final implementation through visibility rasterization, reconstruction, material compilation and registration, evaluation/shading classification, indirect dispatch, frame-graph dependencies, database uploads, renderer integration, and sample material migration. Superseded intermediate implementations are not reported as current defects. Tests were excluded from the correctness/design review; test call sites were inspected only where needed to establish whether production interfaces were test-specific. Cooked binary assets were not independently decoded.

There are **two correctness findings**, followed by six refactoring opportunities. Both findings are edge cases; the second concerns an incomplete resize path and explicitly distinguishes the older minimize-handling gap from the new code. The findings and proposed refactorings have not been implemented. The only final repository change from this review is this report.

## Correctness findings

### F1 — [P2] Initialize debug color when no object producer is active

Locations:

- `Modules/Graphics/Framework/Private/Graphics/Passes/VisibilityPass.cpp:70–80`
- `Modules/Graphics/Framework/Private/Graphics/Passes/SurfaceEvaluationPass.cpp:41–50`
- `Modules/Graphics/Framework/Private/Graphics/RendererImpl.cpp:238–245`

**Trigger:** Keep a view enabled with its rendering view modules installed, then remove or deactivate all object scene modules. Select `visibility`, another reconstruction debug mode, or `evaluation-group`.

`SurfaceEvaluationInputs` is then empty. Visibility/reconstruction debug writes the main color target only inside the producer loop. Evaluation-group debug has the same dependency on a producer loop. The renderer's fallback tests for installed view stages, so it does not clear the target in this case. Visibility still clears its own visibility/depth attachments, but those clears do not initialize main color. Tonemapping subsequently samples the newly allocated main color texture with undefined contents. Pool reuse can make this appear as stale output from another frame or view.

This specifically requires **no active producer**. An active MeshSceneModule with zero instances still publishes a batch and does not take the same unwritten-output path.

**Suggested change:** Initialize the debug color target once before producer-specific debug passes, or provide a generic background pass when the producer list is empty. Keep producer shaders responsible for their own covered pixels. Apply the rule to both reconstruction debug and evaluation-group debug.

**Evidence:** Direct control-flow/resource-write trace. This scenario was not exercised interactively during this review.

### F2 — [P2] Handle zero surface extent before swapchain recreation

Locations:

- `Modules/Graphics/Core/Private/Graphics/Core/Vulkan/Viewport.cpp:258–276`
- `Modules/Graphics/Core/Private/Graphics/Core/Vulkan/Viewport.cpp:210–218`

**Trigger:** Minimize a window on a platform/driver reporting a fixed `currentExtent` of zero while the swapchain needs recreation.

The changed fixed-extent branch copies `currentExtent` directly into `m_desc`, including zero, and passes those dimensions to `vkCreateSwapchainKHR`. Vulkan requires both swapchain extent dimensions to be nonzero. The acquire retry loop recreates immediately and provides no unavailable-surface result for the renderer to handle. The application continues rendering after polling events, so there is no higher-level minimized-window guard protecting this path.

**Suggested change:** Check for a zero extent before retiring/replacing the swapchain. Propagate a temporarily unavailable result to the renderer and skip rendering/presentation until a nonzero surface is available. Continue processing window events between attempts; waiting for restoration inside the acquire retry loop can prevent the application loop from processing those events. Also avoid creating frame-local render targets at zero resolution.

**Attribution:** The new Stage 5 branch unconditionally consumes zero `currentExtent`; the absence of general minimize handling already existed before this range. This is a gap in the changed resize path, not a claim that the previous renderer handled minimization correctly.

**Evidence:** Static trace against the surface-extent contract. The [Vulkan swapchain specification](https://docs.vulkan.org/refpages/latest/refpages/source/VkSwapchainCreateInfoKHR.html) specifies the nonzero requirement (`VUID-VkSwapchainCreateInfoKHR-imageExtent-01689`); the [Khronos recreation tutorial](https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/04_Swap_chain_recreation.html) also covers minimized zero-sized framebuffers. A minimize/restore run was not performed.

## Refactoring and simplification opportunities

### R1 — Give classification steps focused parameter contracts

Locations: `Shaders/Shaders/Deferred/Tiling.h:24–57` and `Private/Graphics/Passes/SurfaceWorkBuilder.cpp:125–235`, both under `Modules/Graphics/Framework/`.

All classification kernels share one 124-byte constants structure containing descriptors for unrelated steps, multiple mode flags, and an unused `m_g0` UAV. `BuildSurfaceWork` combines a boolean mode with nullable view/visibility pointers and optional count/scatter pipelines. Its mandatory `g0` argument is even used for a resolution assertion in the evaluation path, where classification actually consumes visibility.

Follow the stage-specific input/output approach already used by GpuMeshWorkBuilder: small clear, classification, scan, propagation, and command-emission contracts. Give evaluation and shading classification explicit input descriptions and share the scan/emission implementation. This makes required resources visible in the API and removes combinations that silently classify nothing when evaluation uses the default shaders without a producer header. The unused descriptor can be removed independently.

### R2 — Reuse technique lookup and simplify pipeline bookkeeping

Locations: `Modules/Graphics/Framework/Private/Graphics/Materials/MaterialInstance.cpp:226–246,283–299` and `Public/Graphics/Scene/ComputePassRegistry.h`.

`GetComputePipeline` uses `FindTechnique`, while `GetPipeline` repeats its role/permutation search. Reusing the existing helper is a small, direct cleanup. Separately, ComputePassRegistry maintains parallel pipeline/reference-count vectors that must grow and shrink together. A slot record containing both fields would express their shared lifetime and reduce bookkeeping. Keep the existing registration reference counting, vacant-slot reuse, and revision behavior.

### R3 — Share the pixel-work decoder

Locations: `Modules/Graphics/Framework/Shaders/Shaders/Deferred/EvaluateCommon.hlsli:11–30` and `DeferredCommon.hlsli:11–28`.

`LoadEvaluationPixel` and `LoadShadingPixel` repeat the range check, wave-command load, valid-lane check, packed pixel decode, tile reconstruction, and resolution check. A common helper can return the decoded pixel and command/group metadata; thin evaluation/shading wrappers can supply their own constants. This keeps tile-addressing changes synchronized without mixing the stages' surface contracts.

### R4 — Resolve mesh/material context once per evaluated pixel

Locations: `Modules/Graphics/Framework/Shaders/Shaders/Materials/EvaluateStandardPBR.cs.hlsl:36–40,62`, `EvaluateTextured.cs.hlsl`, `EvaluateMesh.hlsli:5–11`, and `Passes/Visibility/Reconstruction.hlsli`.

Reconstruction resolves instance/group/LOD data, each textured evaluator resolves instance/group/material data again, and `SurfaceShadingRoute` resolves group/material again. A focused mesh-specific evaluation context could carry the material pointer, instance pointer, and shading slot alongside reconstructed attributes. Keep this in the mesh producer, preserving generic passes' opaque object-data interface. Inspect generated shader code before attributing a performance gain: the compiler may already eliminate some repeated loads.

### R5 — Share debug-mode values between host and shaders

Locations: `Modules/Graphics/Framework/Public/Graphics/Passes/VisibilityDebug.h`, `Private/Graphics/Passes/VisibilityDebug.cpp`, `Shaders/Shaders/Deferred/SurfaceDebug.cs.hlsl:42–65`, and `Shaders/Shaders/Passes/MeshPass/VisibilityDebug.cs.hlsl:32–51`.

The host enum, parser name table, shader numeric comparisons, and ordinal stage checks independently encode the same ordering. Adding or reordering a mode can select a different shader branch without a compile error. Put the values in a shared host/shader contract, use named shader values, and centralize the host predicates for required stages. The duplicate ID-color hash is another small helper that can be shared.

### R6 — Size command-emission work from actual coverage

Locations: `Modules/Graphics/Framework/Private/Graphics/Passes/SurfaceWorkBuilder.cpp:99–107,235` and `Shaders/Shaders/Deferred/EmitSurfaceCommands.cs.hlsl`.

The emission dispatch uses the worst-case wave capacity even when only a few pixels are covered. Each useful wave also performs binary searches over bucket offsets and shader bindings. An optimization candidate is a small per-shader range pass plus indirectly dispatched wave emission using the scanned total wave count. Retain the safe allocation bound. Measure sparse scenes and high binding counts before adopting this: an extra pass may cost more for small workloads. No timing improvement is claimed here.

## Production/test separation audit

The audit covered the Framework, Graphics, and GraphicsCore production trees, their build source lists, reflection ownership, and relevant callers. Under the clarified rule—retain interfaces that are generally reusable—**no non-reusable test-specific implementation was found that warranted removal**.

The following stay in production:

- Buffer readback invalidation: a reusable host/GPU memory-coherency operation, including its Vulkan implementation.
- Signed and unsigned color clears: reusable typed render-target operations.
- CPU visibility encoding and shared packing/limit contracts: reusable format utilities, even where present callers are tests.
- Compute subgroup selection, indirect compute dispatch, and transitive buffer dependencies: these also have actual rendering callers.
- Visibility/GBuffer debug modes and validation switches: usable runtime diagnostics exercised by samples as well as tests.

Probe shaders, probe pass descriptions, and graphics test entry points live under `Modules/Graphics/Framework/Tests` and belong to FeGraphicsTests. They are not sources of FeGraphics. Generic Framework APIs such as input-backend injection, diagnostics, and command construction were retained as reusable runtime interfaces. Framework itself has no source changes in the reviewed commit range.

No production or test source edits remain from this review.

## Other contracts checked

- The scene producer namespace, instance-plus-one encoding, meshlet/primitive packing, and background sentinel agree between the current producer and consumers.
- Evaluation and shading registries remain independent; routing rebuilds track registry revisions, mesh changes, and reserved group-row counts. Vacant shader slots and unsupported routes are handled deliberately.
- Hierarchical scans propagate pixel and wave prefixes; the wave allocation bound conservatively allows partial waves for nonempty buckets.
- Producer geometry is retained through declared buffer dependencies for rasterization and reconstruction. Transitive registration merges shader stages and terminates repeated visits, including cycles.
- Generated reflection is consistent with the current MeshViewData/pass contracts. Database and material pages still use explicit transitions for data reached through raw addresses.
- GBuffer coverage controls payload reads; the optional emissive plane is written/read under the same validity bit. Tonemapping performs sRGB encoding into UNORM swapchain attachments.

These checks produced no additional confirmed correctness findings. They are source-review conclusions, not exhaustive GPU correctness proofs.

## Validation and limits

- `configure.bat`: passed after restoring the original source state.
- Reflection generation completed; it produced no final tracked changes.
- Debug builds passed for `FeFramework`, `FeGraphics`, `FeGraphicsCore`, `RendererSample`, and `GameSample`.
- `RendererSample --smoke --api-validation` and `GameSample --smoke --api-validation` each completed eight frames and exited with code 0. Neither run logged a validation error or warning. Hardware: NVIDIA GeForce RTX 4060 Laptop GPU. These runs exercised default deferred rendering, not the two edge cases above.
- Configuration/build emitted existing CMake deprecation, MSBuild duplicate-source-basename, and compiler exception-option warnings; there were no build errors.
- The two reported edge cases were identified by static tracing; neither was reproduced interactively.
- Tests were not reviewed. No tests were added or changed in the final result. No cross-vendor performance/residency measurements or manual asset recooking were performed.

## Implementation follow-up — 2026-10-17

- **F1:** Reconstruction and evaluation-group debug initialize color once before producer-specific work. Producers only write their own covered pixels. Empty producer lists no longer create descriptors for an unsubmitted group-debug pass; the unused surface-debug depth descriptor was also removed. A GPU regression probe poisons the HDR target, runs installed visibility/evaluation stages with no producers, and checks every output pixel in visibility, normal, depth-error and evaluation-group modes.
- **F2:** Image acquisition reports temporary unavailability before the renderer begins a graphics frame or allocates targets. Swapchain creation checks the resolved extent before releasing old resources; recreation is deferred to acquisition. Acquisition makes one attempt and yields to the application loop on an out-of-date result. Per-acquire semaphore completion fences preserve safe semaphore reuse now that acquisition precedes `BeginFrame`. A native-window regression probe verifies rendering, minimize/suspension without advancing the queue frame, and successful restore.
- **R1:** Evaluation and shading have explicit input structs and overloads, with mandatory producer classification pipelines for evaluation. Clear, count, scatter, scan, propagation and emission use focused reflected contracts. The boolean classification mode, unused G0 UAV and evaluation's G0 dependency were removed. Routing upload, clearing, scanning and emission are separate helpers.
- **R2:** Graphics/compute technique lookup shares `FindTechnique`. Compute registry entries store the pipeline and reference count together, retaining duplicate registration, vacant-slot reuse, trailing-slot removal and revision behavior. Existing registry tests also check revision stability across non-final releases.
- **R3:** A shared pixel-work decoder returns the pixel and wave-command metadata; evaluation and shading retain thin stage-specific wrappers.
- **R4:** Mesh evaluation retains the resolved instance/group handles and loads one context containing reconstructed attributes, material/instance parameter pointers and shading route. Generic passes still consume opaque object data. No material-evaluation speedup is claimed; generated shader assembly was not compared.
- **R5:** One host/shader list defines debug mode values and command-line names. Shader branches use named values, host stage selection uses centralized predicates, and debug shaders share the ID-color hash.
- **R6:** The root prefix scan writes emission dispatch arguments from the actual wave total, also allowing enough threads to initialize every shader range. Emission dispatches indirectly. This requires one 12-byte scratch buffer and no extra pass, while preserving the conservative wave allocation bound and existing wave-to-bucket searches.

### Emission measurement

The opt-in graphics probe compares conservative direct dispatch with actual-count indirect dispatch using Vulkan timestamps, alternating variant order. Each reported value is the median of 16 samples, each containing 16 dispatches, divided by the batch size. Validation was disabled for measurement. Hardware: NVIDIA GeForce RTX 4060 Laptop GPU.

| Workload | Bindings | Direct emission | Indirect emission |
| --- | ---: | ---: | ---: |
| 257×129, empty coverage | 512 | 1.664 µs | 1.664 µs |
| 257×129, dense coverage | 512 | 3.712 µs | 3.712 µs |
| 2049×1281, dense coverage | 512 | 16.768 µs | 16.704 µs |
| 2049×1281, approximately 1% coverage | 512 | 7.168 µs | 6.592 µs |

The sparse case improved by about 8%; dense/small cases were effectively unchanged. These are repeated emission-dispatch timings, including their inter-dispatch dependencies, not whole-frame or end-to-end classification timings. The new root-scan argument write is outside the measured interval. No cross-vendor result is implied. The proposed additional per-shader range pass was unnecessary for this change and was not adopted.

Run `FeGraphicsTests --gtest_filter=VisibilityGpu.* --benchmark-surface-emission` from the graphics tests build directory to repeat the measurement. Benchmark code and the direct-dispatch comparison contract remain under `Tests`.

### Verification

- Reflection generation and `configure.bat` completed.
- Debug builds passed for `FeGraphicsTests`, `GameSample`, `RendererSample` and `GpuDrivenTestApp`.
- All seven graphics tests passed with GPU validation, including empty-producer output, minimize/restore, registry lifetime/revision checks, and emission argument readback. The opt-in emission benchmark also passed its pixel/dispatch correctness checks.
- `GameSample --smoke --gpu-validation` passed in deferred, normal reconstruction and evaluation-group modes; `RendererSample --smoke --gpu-validation` passed.
- `GpuDrivenTestApp --gpu-flight-stress --gpu-validation --frames 15` passed.
- No Vulkan validation errors were reported in those runs. Existing validation-layer configuration warnings and build warnings remain.
- No assets were recooked. Changes remain uncommitted.
