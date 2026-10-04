# GPU-driven test application

The renderer classifies visibility once per view, then prepares eligible mesh work separately for each pass. CPU collection traverses spatial batches and uploads batch IDs and member offsets. Persistent membership, transforms, residency, material routing, and geometry stay in GPU tables. Clear, instance classification/counting, hierarchical bucket scans, command construction, and scatter run on the GPU. Depth and opaque consume independent buckets through amplification-shader meshlet culling and payload-driven mesh shaders.

Preparation shaders are separate files under `Shaders/Passes/MeshPass`; each pass has its own shared C++/HLSL header, push constants, and resource declarations. Common frustum helpers live in `Shaders/Core/Culling`. There are no pass-selection defines.

## Run

Build `GpuDrivenTestApp` in the Windows Debug preset. Run from the sample build directory because the existing shader source cache resolves paths relative to it:

```powershell
Push-Location cmake-build/windows-debug-msvc/Samples/GpuDrivenTestApp
& ../../Debug/GpuDrivenTestApp.exe --gpu-stress
Pop-Location
```

The default mode and `--gpu-stress` run 15 frames through the production renderer. The sample exercises mixed meshes/materials, partial culling groups, deletion/row reuse, stable batch indices and row-ID indexing, buffer growth, independent pass tags, disabled passes, mirrored/sheared/degenerate transforms, migration, streamed LOD changes, mesh/material reload, missing techniques, rejected candidates, and an empty scene. This mode waits for the GPU between frames.

Set `$env:VK_LAYER_VALIDATE_SYNC = "1"` in the shell running the sample to enable synchronization validation.

`--gpu-flight-stress` runs the same lifecycle changes without per-frame GPU waits. Use Vulkan core, synchronization, and GPU-assisted validation to inspect ownership, barriers, and accesses with frames in flight.

`--gpu-benchmark` keeps the scene unchanged and measures renderer frame time, including CPU submission and the GPU completion wait. It discards four warm-up frames and reports an average. This is a sample-side wall-clock measurement; the Graphics modules contain no benchmark queries or counters. Its default duration is 64 frames. `--frames N` overrides any mode's duration.

Select benchmark workloads with `--gpu-all-culled`, `--gpu-small-meshes`, and `--gpu-large-meshes`. RendererSample keeps the default textured scene; this application owns all validation modes. `--instances N` changes the stress workload (clamped to 257 through 16,384); for a large-mesh paging run, use `--gpu-benchmark --gpu-large-meshes --instances 2049 --frames 6`.

## Preparation passes

`GpuMeshWorkBuilder` is a namespace of frame-graph helpers. View culling, per-pass eligibility counting, clear, hierarchical scan, offset propagation, argument finalization, and scatter each record their own pass. Finalization writes every reserved command page, including zero arguments for unused pages, so clear only resets counts and cursors. The scan passes declare only scan buffers; scatter reads classification directly without loading scene tables.

Preparation uses one-dimensional compute dispatches with no dispatch-width constants or compute group-count checks. The renderer targets Turing and RDNA2 or newer GPUs and assumes the 256-thread compute and 32-thread task groups fit. Indirect mesh draws use fixed pages of 65,535 task groups, within the [Vulkan mesh-shader minimum limits](https://docs.vulkan.org/refpages/latest/refpages/source/Required_Limits.html). Each command stores only its first work chunk and chunk count. The capability-query structure and entity-count overflow assertions have been removed; Vulkan allocation still validates storage-buffer ranges.

Preparation always uses the explicit hierarchical scan. The experimental fused path and its completion counter have been removed.

Debug callbacks, readbacks, culling overrides, forced dispatch/page limits, shuffled dispatch order, registry padding, and preparation timing counters have been removed. Validation uses the production renderer and Vulkan validation layers. RendererSample, GpuDrivenTestApp, and FerrumCli builds pass. Default Renderer rendering, both 15-frame lifecycle stress modes, and the small-mesh and all-culled benchmarks completed without reported Vulkan errors or memory leaks. Flight validation initially encountered an intermittent asynchronous I/O allocator crash; a retry passed. The 2,049-instance highest-detail paging workload currently returns `VK_ERROR_DEVICE_LOST` with GPU-assisted validation on the test machine.

## Assets

This application owns its asset depot, including material variants, triangle geometry, and the palette shader under `Assets/source/Shaders`. CMake stages that shader into the build directory for the shader source cache. Assets are rebuilt with FerrumCli. Mesh payloads now append one local-space float4 sphere per meshlet after packed triangles. Submesh bounds cover all LODs, and LOD error metadata includes the original highest-detail LOD. `Triangle` supplies a single-meshlet mesh; `BunnyOpaqueOnly` and `BunnyDepthOnly` exercise missing techniques.

Rebuild modified assets through `ferrum build`, supplying `Samples/GpuDrivenTestApp/Assets/source` as the source root and `Samples/GpuDrivenTestApp/Assets` as the output root. FerrumCli appends the `artifacts` directory itself.

## Batch membership

Each spatial batch is limited to 16,384 instances and owns a persistent `MeshMemberTable` slice containing instance references and one `MeshBatchTable` row with that slice and its full CPU draw-tag mask. Accepted batches emit `{batchId, firstMember}` entries, one per group of up to `kInstancesPerCullGroup` instances. Culling indexes the batch slice at `firstMember + lane`; no persistent cull-chunk table or fixed 256-row membership allocation remains.

Generated DB tables expose `TryReallocateRows(slice, count)`. It resizes in place when the old and new counts share a power-of-two allocation size, preserves existing rows, and initializes newly exposed rows. Failure leaves the slice untouched. Resizing to zero releases it. Batch updates allocate a replacement and rewrite membership when the allocation size class changes. The lifecycle stress modes also check this API's growth, shrink, initialization, failure, empty-slice behavior, and the single-page size limit. Database slices and column copies stay within one page.

CPU batch indices remain stable for each batch's lifetime. The batch table row ID indexes the CPU pointer array and the octree entry. A pointer array retains vacant slots as null pointers, and the table allocator reuses freed IDs. The lifecycle stress modes delete a middle slot, verify surviving indices, verify replacement indexing by its allocated row ID, and leave holes for shutdown cleanup.

Batch bounds and membership dirtiness are bit vectors indexed by batch table row ID. Scene updates traverse only dirty batches; destroying a batch clears pending bits before its row can be reused. Live mesh groups have a separate bit vector, so group updates and rendering skip vacant table slots. Live groups still poll asset generations and mesh buffer/LOD changes because asset reload and residency changes do not expose scene notifications. The lifecycle stress sample also discards a dirty batch before updating and checks membership counts after every frame.

Material techniques explicitly select amplification shaders and attachment formats through `renderTargetFormats` and `depthTargetFormat`. The sample's opaque techniques use `B10G11R11_UFLOAT`; depth-only techniques have no color attachments, and both use `D32_SFLOAT_S8_UINT`. Pipeline creation uses those properties without role-based format overrides. Each material runtime caches pipelines by technique role, and the scene shares one routing registry per technique role across views. Passes supply their actual CPU draw-tag mask as two 32-bit words; shaders do not hard-code tags or technique roles. Renderer scheduling retains explicit depth-then-opaque calls and view-module blackboard markers.

The main color target uses `B10G11R11_UFLOAT` independently of the swapchain. Blit samples it as floating-point RGBA (implicit alpha 1) and selects its pipeline from the destination format, so the swapchain's UNORM conversion occurs on the output write. Values outside the destination range are clamped; Blit does not tone-map.

The lifecycle stress modes configure the second view with the `MaskProbe` technique and a statically allocated tag above bit 31. They alternate batch eligibility for this pass independently of depth and opaque, exercising both words of the mask and arbitrary technique roles.
