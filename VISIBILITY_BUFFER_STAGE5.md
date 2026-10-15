# Stage 5: default visibility/deferred pipeline

RendererSample, GameSample/WorldGraphicsSceneService and GpuDrivenTestApp now install three view modules: `Visibility::ViewModule`, `SurfaceEvaluation::ViewModule` and `DeferredShading::ViewModule`. Their `AddPasses` functions take only the graph and blackboard. Visibility publishes its raster target and geometry view buffer; surface evaluation publishes G0–G3. The passes create transient visibility/GBuffer targets in `AddPasses`; the graph pool commits and reuses their storage, and blackboard outputs pass them to later stages. The modules own only the persistent classification state independently for each view. Main HDR and depth remain renderer-owned, with the existing last-view compositing behavior.

Default rendering executes all three stages. `--visibility-debug` still selects raster/reconstruction and GBuffer views, or `deferred` explicitly. The forward mode, depth-only/opaque passes, material techniques, material pixel shaders, sample shader copies and old draw tags have been removed. Sample material definitions contain visibility, evaluation and shading techniques only. The lifecycle fixtures formerly named BunnyOpaqueOnly/BunnyDepthOnly are now BunnyNoVisibility/BunnyUnsupported, preserving their asset identities while testing absent visibility or evaluation roles.

RDNA2+ and Turing+ are the hardware baseline. VisibilitySupport and the unused format-capability query API are gone. HDR is B10G11R11_UFLOAT, depth is D32_SFLOAT, visibility is R32G32_UINT and GBuffer planes are R32_UINT. Wave-size selection/control remains. No additional capability checks were added.

Compute registries reference-count live group registrations, retain shared shaders while groups use them, and reuse vacant slots after reload/stream-out/destruction. Graphics routing drops obsolete pipeline slots when rebuilt. Shader ranges and arguments are rebuilt for the current routes; unsupported combinations produce magenta and a material-generation warning. Resource references captured by the graph and existing queue/pool fence retirement retain resources used by submitted work. Dirty database-page synchronization remains unchanged.

The two classifiers share coordinate/command storage only within a view. Classification buffers are committed by the graph pool from declared copy, shader and indirect-argument accesses; the builder does not precommit memory or request extra usage flags. Logical capacities now determine the shared allocation, allowing buffers to shrink after bindings disappear; accounting handles shrinking without unsigned underflow. Each graph requests images at the current resolution; resizing resets each view's classification state. Swapchain recreation follows the actual window extent and reacquires an image after an out-of-date result.

## Validation

Debug builds, configure.bat and reflection generation pass. The three material depots were rebuilt manually through FerrumCli, including source-less material instances and the GameSample world.

- Seven graphics tests pass with GPU-assisted validation, including shader registry reference counts/slot reuse, independent classification, GBuffer lighting, and shared-scratch shrinkage.
- All 24 GameFramework tests and the standalone reflection-link executable pass.
- Both samples pass default-rendering smoke runs with API validation.
- The 15-frame GPU flight stress run passes with GPU-assisted validation and no validation errors. It exercises two independently filtered views, view-module deactivation/reactivation, batch/instance churn, material replacement, LOD streaming, mesh/material reload, unsupported routes, empty views, and two window resizes. Allocation logs confirm 2560x1351 -> 960x640 -> 1280x720 targets and scratch resets in both views.

## Footprint and timing

These are requested resource bytes recorded by the implementation, before image alignment/compression, pool reservations, streamed assets, GPU database, swapchain and overlap with retired generations. Scratch includes classification buffers and counts shared pixel/wave storage once per view. They are not a measurement of driver VRAM residency.

| Run / resolution | Visibility + GBuffer per view | Shared HDR + depth | Classification scratch per view |
| --- | ---: | ---: | ---: |
| GameSample, 1280x720 | 22,118,400 | 7,372,800 | 4,437,568 |
| RendererSample, 2560x1351 | 83,005,440 | 27,668,480 | 16,266,600 |
| Flight stress peak requested sets, 2560x1351 | 83,005,440 | 27,668,480 | 16,277,100 |
| Flight stress resized, 960x640 | 14,745,600 | 4,915,200 | 2,968,524 |
| Flight stress empty, 1280x720 | 22,118,400 | 7,372,800 | 11,060 |

The stress run has two views, so its largest requested current image/scratch sets total 226,233,560 bytes; HDR/depth are counted once. Actual transient peaks can be larger while earlier generations remain in flight. Optional G3 payload access remains guarded by its validity flag; the image set reserves all four planes.

On the NVIDIA GeForce RTX 4060 Laptop GPU, `GpuDrivenTestApp --gpu-benchmark --gpu-small-meshes --frames 32` recorded **14.204 ms** average with graph-pooled transient targets across 28 measured frames after four warmup frames, at 2560x1351 with two views and 513 stress instances plus the authored sample objects. This was a Debug build with the existing debug shader compilation policy, FIFO presentation, no validation, and a device wait included in the timer. It measures CPU submission plus GPU/presentation wait, not individual GPU stages. It is not directly comparable to the earlier 0.6 ms classification capture.

GPU marker scopes distinguish DepthVisibility, SurfaceEvaluation/SurfaceClassification and DeferredShading/ShadingClassification. Per-stage GPU timings, bandwidth counters, physical allocation peaks and extended visual inspection still require a profiler capture; none are inferred from the CPU frame timer.
