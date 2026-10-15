# Visibility buffer and compute deferred shading migration

Status: Stages 1 through 5 are implemented. Visibility rasterization, GBuffer evaluation and independent deferred compute shading are the default and only material pipeline. Each stage has a view module and exchanges resources through the FrameGraph blackboard. See `VISIBILITY_BUFFER_STAGE5.md` for lifecycle validation, requested image/scratch footprints and the measured frame benchmark. Per-stage GPU captures and visual review remain manual checkpoints.

## 1. Direction

Replace opaque forward shading with this sequence:

```text
Scene/material uploads and existing GPU mesh culling
  -> depth prepass + 64-bit visibility output
  -> loose tiling by material evaluation shader, tile, and mesh/material group
  -> compute material evaluation -> packed surface GBuffer
  -> loose tiling by deferred shading kernel and tile
  -> compute shading -> existing HDR target
  -> existing ACES tonemap and presentation
```

Make the depth prepass write visibility at the same time. This fulfills both depth and visibility production with one geometry traversal; a second depth-equal visibility traversal is only a temporary migration option if needed for comparison. Keep reverse-Z, mesh shaders, bindless textures, the GPU database, and asset streaming.

The first loose-tiling stage evaluates surfaces, including textures and normal maps. The second independently classifies the resulting surface records and prepares lighting commands. All required materials use both stages, including non-PBR materials. There is no permanent forward-shading escape path for the sample materials.

Initial scope is the samples' single-sample opaque geometry. Transparency, new alpha-masked materials, skinning/deformation, motion vectors, VRS, clustered lights, decals, and async-compute scheduling are separate extensions. Their absence must not silently change the existing opaque materials or their supported permutations.

## 2. Current implementation and migration implications

Paths below are repository-relative.

| Area | Current implementation | Implication |
| --- | --- | --- |
| Frame orchestration | `Modules/Graphics/Framework/Private/Graphics/RendererImpl.cpp`: uploads scene/database/material data; clears targets; prepares the view; runs `DepthPrepass`, `OpaquePass`, then tonemaps. | Replace the opaque pass sequence here, preserving empty-scene and per-view behavior. |
| Depth/HDR | Main depth is `D32_SFLOAT_S8_UINT`, cleared to zero. Main color is `B10G11R11_UFLOAT`. Sample depth techniques use greater/write; opaque techniques use equal/write. | Use `D32_SFLOAT` when stencil is no longer needed; retain the 4-byte HDR format if storage writes are supported. |
| GPU work | `Scene/GpuMeshWorkBuilder.cpp` and `Passes/MeshPass/*` cull members, count/scan/scatter work into pipeline buckets, and issue indirect mesh dispatches. `MeshCull.as.hlsl` then culls meshlet spheres. | Reuse culling and draw submission. Screen-space pixel classification is a separate builder, not another mesh-work bucket. |
| GPU scene | `Features/Mesh/MeshSceneModule.cpp`, `Gpudb/Meshes.gpudb`, and generated tables hold instance, group, LOD, material, batch and membership data. | Visibility should identify a database instance, not a transient draw slot or `SV_InstanceID`. |
| Geometry | `OpaquePass.ms.hlsl` reads meshlet headers, a meshlet vertex-remap array and packed local triangles. It currently exports the complete material vertex attributes even for the shared depth mesh shader. | Introduce a minimal position/ID mesh shader. Compute reconstruction must follow the same indirections. |
| Materials | `MaterialTechniqueDesc` and `MaterialInstanceRuntime` expose graphics techniques; runtime parameter reflection reads pixel shaders only. | Add explicit compute material techniques and reflect their parameter layouts. |
| Compute support | `PipelineFactory` already creates compute pipelines; frame graph descriptors already describe compute resource reads/writes. `FrameGraphContext` exposes direct compute dispatch and indirect mesh dispatch, but no indirect compute dispatch. | Extend the existing Core abstraction and Vulkan backend; do not duplicate the pipeline factory. |
| Sampling | `Shaders/Base/Descriptors.hlsli` already exposes `SampleGrad`. | Reuse it with analytically reconstructed derivatives. |
| Lighting | `Core/Shading/PBR.hlsli` supplies one fixed directional light; no indirect illumination. `Textured` and bunny palettes implement their own Lambert-like lighting. | Preserve these distinct equations and color/exposure conventions in separate deferred kernels. |
| Sample integration | Renderer adds depth/opaque view modules directly. GameFramework does so in `Services/WorldGraphicsSceneService.cpp`; `Graphics/MeshSceneBridge.cpp` sets draw tags. | Update both integration points. Generic `Modules/Framework` needs no rendering changes. |

### GPU identity and geometry details

The existing lookup is:

```text
MeshInstanceTable row
  -> m_meshGroup -> MeshGroupTable row
       -> m_geometry: device address of the current resident LOD's geometry
       -> m_lods: currently a one-row slice selecting that resident LOD
       -> m_materialInstance -> MaterialInstanceTable row -> parameter pointer
  -> m_transform
  -> m_instanceData: per-instance parameter pointer
```

`MeshMemberTable` connects batch membership to these instance rows. `VisibleInstance` also contains the instance row and selected LOD row, but its position in the compacted work list is temporary. `MeshBatch` is a scene/culling organization and can contain different mesh/material groups; it is not McLaren's uniform material batch group.

`UpdateGroup` currently exposes submesh zero and one resident LOD through the group pointer. This migration must preserve that actual contract, rather than assume an arbitrary model-wide triangle buffer or per-view LOD selection already exists. Group geometry, LOD metadata and material routing must describe the same frame snapshot until all its consumers finish.

### Required material coverage

| Material/assets | Preserve in material evaluation | Preserve in deferred shading |
| --- | --- | --- |
| `Samples/Renderer/Assets/source/Textured.mat.luau`, `BunnyMaterial.asset` | Albedo texture, `Tint`, per-instance `UvScale`, and `TwoSided` raster permutation. | `texel.rgb * tint.rgb * (0.15 + 0.85 * max(dot(N, normalize(0.4, 0.7, -0.2)), 0))`. |
| `BunnyVariants.mat.luau`, `BunnyWarm.asset`, `BunnyCool.asset` | `Palette=0/1` and their exact authored warm/cool linear colors. | The same non-PBR directional/ambient equation as today. |
| Both sample depots' `Helmet/StandardPBR.mat.luau` and `DamagedHelmetMaterial.asset` | Base color, linear ORM, BC5 normal map, emissive, per-instance `UvScale`, and `TwoSided`. Preserve texture color-space and tangent conventions. | Existing GGX/Smith/Schlick/Lambert behavior, 10,000-lux light, 1,000-cd/mР’Р† emissive scale and EV100 12 pre-exposure. AO remains reserved for indirect lighting, not multiplied into direct light. |

The sample techniques are opaque with blending disabled. `Textured` returns alpha today, but the RGB HDR target does not retain it; do not introduce opacity or cutout semantics during migration. `TwoSided` currently disables culling without automatically flipping shading normals; preserve that behavior. Also provide a non-PBR surface mapping for the generic `OpaquePass.ps.hlsl` constant-color shader if it remains reachable.

## 3. Depth and thin visibility contract

Use `R32G32_UINT`, exactly 8 bytes per pixel, without blending, filtering, barycentrics, UVs, normals or material IDs in the attachment.

| Word | Encoding |
| --- | --- |
| X | `MeshInstanceTable` row index + 1; zero means background. Reserve the maximum row index so addition cannot wrap. |
| Y | `(meshletIndex << 6) | localPrimitiveIndex`, with 26 bits for meshlet index and 6 bits for the triangle inside that meshlet. |

This is a primitive identifier in the engine's meshlet representation, not an index into a conventional triangle list. Current meshlet limits are 64 vertices and 64 primitives. Validate encoding limits against cooked mesh metadata; never silently truncate. If meshlet limits change, version this contract and shader definitions together.

Clear visibility to integer zero and depth to reverse-Z zero. Ensure the render-target clear path handles integer attachments correctly; its current public clear interface is float-color based. Zero has matching zero bits, but add a typed clear representation before supporting arbitrary integer clear values.

The mesh shader carries the actual meshlet index from the amplification payload and explicitly emits a per-primitive encoded ID. The pixel shader writes that ID and the persistent instance row. Do not rely on hardware primitive numbering across mesh workgroups, culling, or indirect commands. Preserve source triangle winding and explicitly use flat/per-primitive interpolation for IDs.

The depth/visibility pass performs identical position transforms to reconstruction, keeps the current cull permutation, writes depth with greater comparison, and outputs visibility only for winning opaque fragments. This naturally resolves equal-depth ownership once; a separate equal-depth replay can otherwise choose a different coplanar winner. Use a position-only mesh path for these opaque materials; none requires texture sampling to determine coverage.

Reconstruct geometry as follows:

1. Decode the instance row, group and resident LOD metadata.
2. Locate sections relative to `group.m_geometry`: vertices, 32-bit meshlet vertex-remap indices, meshlet headers, packed triangles, then bounds.
3. Read the header at `meshletIndex` and the packed triangle at `header.m_primitiveOffset + localPrimitiveIndex`.
4. Its three indices are meshlet-local. Read each global vertex index from the remap array at `header.m_vertexOffset + localIndex`.
5. Load only attributes the material needs and apply the instance transform.

Do not add a triangle-to-meshlet table or a duplicated full triangle-index buffer merely to undo meshlet packing. Reuse the existing geometry layout and device addresses.

No LOD bits are necessary while the current one-resident-LOD-per-group contract holds. Before adding independently selected per-view LODs, extend visibility resolution with a per-view instance-to-geometry/LOD record or a revised ID scheme. Never infer an old frame's LOD from mutable current residency.

Keep rows, geometry buffers, material parameter allocations, descriptors and shader routing valid through the frame fence, including asset reload and stream-out. Preserve the existing explicit registration of device-address geometry reads and extend retention/dependencies through compute evaluation. Do not recycle rows or mutate a frame's routing underneath its visibility data.

## 4. Material pipeline and GPU routing

Introduce three explicit roles:

- `DepthVisibility`: graphics coverage and ID output, specialized only for raster/geometry/coverage differences.
- `SurfaceEvaluate`: compute shader and its parameter layouts; resolves geometry/textures and packs surface data.
- `DeferredShade`: compute shader defining the GBuffer interpretation. Multiple material evaluation shaders may share it.

Extend `MaterialTechniqueDesc`, `Materials/engine/technique.luau`, `material.luau`, `AssetBuilder/MaterialProcessor.cpp`, and `MaterialInstanceRuntime`. Distinguish graphics and compute techniques, reject incompatible fields, preserve specializer keys/defines, and add a compute-pipeline accessor. Parameter reflection must include `SurfaceEvaluate` and enforce consistent material/instance layouts across all stages that consume them. Parameter-free palette shaders must continue to work.

Add separate scene-owned per-pass pipeline registries for surface evaluation and deferred shading. Initially group by shader/defines; later refine each pass independently by its consuming material parameters. The parameter allocator does not own routing. Runtime slots are transient, never serialized asset IDs. Deduplicate equivalent PSOs, including equivalent depth/visibility permutations that differ only in texturing or palette selection.

Extend the GPU database schema in `Gpudb/Meshes.gpudb`, then regenerate its C++/HLSL outputs. Store independent evaluation and shading routing slots beside the material parameter reference. Surface feature flags are written by evaluation shaders, not technique metadata. Keep group-to-material lookup intact. The existing material table row is allocated per mesh group, so deduplicate equivalent runtime material bindings if merging them becomes worthwhile; do not assume those row IDs are globally canonical material assets.

Use `MeshGroupTable` row as the initial first-stage uniform group key: it already associates geometry and one material binding, and can span batches with different instances. Wave-uniform material parameters and textures can be broadcast. Transforms, visibility primitive IDs and instance parameters can still differ between lanes. Never broadcast `UvScale` or transforms merely because the material is uniform.

The shading route is derived from the selected deferred shader/defines and, when needed, its material parameters; there is no predefined shading-model enum or technique field. `Textured` and both bunny palettes can share a `SimpleLit` kernel while keeping their evaluation permutations. `StandardPBR` uses a `StandardPBR` kernel. Reserve IDs for background and a diagnostic unsupported-material path; missing routes must be visible and reported, not interpreted as PBR by default.

## 5. First loose-tiling pass: material evaluation commands

Follow McLaren's spatial-locality and uniform-wave design, using 128x128 screen tiles. This is pixel compaction within large tiles, not one 128x128 workgroup or one partly occupied 8x8 shading group per material.

### Work organization

Order work by `(evaluation PSO, tile, mesh/material group)`. Within each tile/group, build consecutive pixel records and split them into logical wave-sized commands. A dispatch for a shader can consume many groups and tiles, with the group uniform within each command.

- Pixel command: initially a 32-bit word containing `localX | (localY << 7)`; upper bits are reserved. Native 16-bit storage or later pair packing can reduce it after measurement. Bounds are checked for partial edge tiles. Do not store full coordinates or visibility IDs again.
- Wave command, initially 16 bytes: tile index, uniform group ID, pixel-list offset, valid pixel count. Each field starts as 32 bits; compress later only with measured benefit and explicit bounds.
- Shader range: first wave command, command count and indirect dispatch arguments. Pixel lists contain actual pixels; tail waves have a valid count instead of up to 63 stored padding pixels.

Support 32- and 64-lane targets through device/pipeline capabilities, rather than hard-code McLaren's console wave64 assumption. A simple correct baseline is one logical command per compute workgroup, all lanes sharing its material. When an exact hardware subgroup size is available, specialize group width accordingly. Otherwise keep group-uniform correctness even if the workgroup maps to more than one hardware subgroup; wave intrinsics must use the actual subgroup layout. Pack using the same logical width consumed by that pipeline.

### Full-view classification and scratch

The current implementation replaces the original bounded-window sorting proposal after performance captures showed severe serial dependencies and low sort occupancy. The user explicitly permits resolution-dependent scratch and practical limits when justified.

1. Map persistent resident group rows to dense binding IDs ordered by evaluation shader.
2. Clear counters/cursors and G0; count pixels by shader/tile/binding using many small workgroups and wave-aggregated atomics.
3. Hierarchically prefix-scan paired pixel/wave counts into ranges, preserving shader/tile/group locality.
4. Reread visibility and scatter one coordinate word per pixel into disjoint reservations.
5. Generate wave commands and shader ranges/indirect arguments, then dispatch each evaluation shader once for the entire view.

Scratch is persistent and reused on the same graphics queue, with reallocation on dimensions/routing changes and fence-safe retirement. For padded view pixels `P`, tiles `T`, dense bindings `G` and lane count `L`, buckets are `B=TG` and safe command capacity is `min(P,ceil(P/L)+min(P,B))`. There is no fixed shader/group limit; buffer sizes are checked against addressable limits. See `VISIBILITY_BUFFER_STAGE3.md` and `VISIBILITY_BUFFER_CLASSIFICATION_OPTIMIZATION.md` for allocation accounting and sparse-mask alternatives.

## 6. Attribute reconstruction and texture filtering

Put geometry access and reconstruction in shared shader helpers, independent of the old pixel-shader entry point and its push constants.

- Reconstruct triangle clip positions using exactly the raster pass's transforms. Evaluate at pixel centers with the same viewport orientation, offsets and reverse-Z projection.
- Calculate perspective-correct barycentrics and analytic screen derivatives. Interpolate UV, normal and tangent; normalize at the same conceptual points as the existing material code.
- For affine screen barycentrics `lambda`, use `q = sum(lambda[i] / w[i])` and `uv = sum(lambda[i] * uv[i] / w[i]) / q`. Differentiate the quotient in X and Y. Apply per-instance UV scale to both UVs and derivatives.
- Use `SampleGrad` for all existing material texture samples. Compacted neighboring lanes are not neighboring screen pixels; implicit derivatives and quad operations are invalid for this command stream, even though the device enables compute-derivative extensions.
- Preserve inverse-transpose/cofactor normal transformation, mirrored-transform tangent handedness, and the existing glTF/MikkTSpace +Y normal-map convention. Keep two-sided shading behavior explicit.
- Handle nearly degenerate triangles and projection singularities without NaNs or unbounded gradients. Use a robust homogeneous/projective formulation or a slow clipping-aware reconstruction path for triangles crossing the camera/near plane; a naive divide by zero followed by arbitrary clamping is not sufficient.

Default to reconstructing attributes inside material evaluation. This avoids full-resolution barycentric, derivative, UV and tangent-frame images. Load no UV/tangent data for constant-color palette evaluation; fetch only position and normal data needed there. Preserve the current interleaved vertex format initially, then consider split streams or a bounded per-tile triangle/vertex cache only if fetch/transform repetition dominates captures.

DOOM's separate interpolation pass reduces shader register pressure, but its published buffers cost 20 bytes per pixel before visibility and GBuffer storage. Treat that as a measured alternative, not the default for this memory-focused migration. A bounded cache is preferable to immediately adopting those full-screen intermediates.

## 7. Packed GBuffer and non-PBR contract

Start with four separately addressable `R32_UINT` planes. Use raw integer loads/stores and shared packing helpers; avoid automatic sRGB conversion or filtered sampling of packed data.

| Plane | Bits | StandardPBR interpretation | SimpleLit interpretation |
| --- | --- | --- | --- |
| G0 | RGB8 + 8-bit shading-model ID | Linear base color manually encoded to sRGB8; decoder identifies PBR. | Unlit texture*tint or palette color manually encoded to sRGB8; decoder identifies SimpleLit. |
| G1 | Two signed 16-bit octahedral coordinates | Final world-space shading normal, after normal mapping. | Interpolated world-space normal. |
| G2 | Four 8-bit fields | Perceptual roughness UNORM8, metallic UNORM8, AO UNORM8, model-specific flags. | Model-local fields; initially unused values set deterministically, plus flags if needed. These bytes are not interpreted as PBR properties. |
| G3 | Packed nonnegative R11/G11/B10 floating-point emissive | Scene-linear emissive radiance, before exposure, using the existing 1,000-cd/mР’Р† scale. | Optional model-specific payload; current SimpleLit does not need it. |

This is a 12-byte base surface plus an optional 4-byte extended plane. Both present sample scenes include an emissive-capable helmet, so budget the full 16 bytes/pixel for them. Do not claim an average 12-byte resident allocation simply because many pixels are non-emissive. If a view has no extended-payload models, omit G3 entirely. Otherwise use an explicit payload-valid flag and skip G3 stores/loads for pixels that do not need it. A zero-emissive PBR pixel can clear that flag without writing G3.

RGB8 is stored inside a 32-bit word with useful metadata, not a poorly supported 24-bit storage-image format. At encode, clamp bounded reflectance to [0,1], apply the piecewise linear-to-sRGB transfer function, then round to 8-bit. At decode, normalize the bytes and explicitly apply the inverse transfer function. Reuse `Core/Shading/Color.hlsli` and add the inverse. Source base-color/emissive textures already use sRGB views; their samples are linear before GBuffer encoding. Do not apply the source-texture transfer twice.

Use the full octahedral fold/unfold and renormalize after decoding, including negative-Z normals. Two 16-bit coordinates favor stable helmet specular highlights. Keep arithmetic in float and quantize only at storage boundaries. Eight-bit perceptual roughness/metallic/AO are a reasonable initial match for these texture inputs; reconstruct `diffuseAlbedo = baseColor * (1 - metallic)` and `f0 = lerp(0.04, baseColor, metallic)` in the PBR shader instead of storing both colors.

The integer G3 representation manually packs the nonnegative floating-point channels, avoiding a requirement for a packed float image UAV just for emissive. Document finite range and sanitize invalid values. Current sample colors and emissive fit this contract. Future materials needing negative values, HDR base color, additional lobes or a different parameter budget must declare a new decoder/payload contract; never silently clamp an arbitrary custom material into PBR semantics.

Only G0's background/model field must be initialized globally. Clear G0 to zero, reserve model zero for background, and write G0/G1/G2 for every covered pixel. Read G1/G2 only for valid model IDs and G3 only when its flag says it was written. This avoids full clears of every GBuffer plane and makes reuse safe at silhouettes and on empty views.

## 8. Second loose-tiling pass and compute shading

Run a second instance of the command builder after GBuffer production completes. Classify from G0's shader-derived shading route, optionally G2 feature flags, rather than going back to visibility, geometry, textures or material assets.

- Key by `(deferred shader/feature permutation, tile, optional uniform shading binding)` and compact actual pixels into wave commands again.
- Current PBR and SimpleLit kernels have only shared view/light constants, so all pixels with the same shading shader can mix within a wave regardless of original material/mesh. Do not retain first-pass mesh-group separation unnecessarily.
- Generate a fresh set of ranges and indirect arguments. This must be a genuine second classification, not reuse of material-evaluation commands under another name.
- Share builder infrastructure and reuse the first stage's scratch allocation after its last read. With very few shading models, a small histogram plus prefix sums can replace the general sparse-sort path here.
- Shade only recorded pixels. Reconstruct world position from sampled reverse-Z depth and inverse view-projection where required; SimpleLit does not need it. Add inverse view-projection and viewport data to the appropriate view constants.
- PBR decodes its fields and reuses the existing BRDF/light equations. SimpleLit decodes its own surface and preserves the current artistic ambient/directional lighting, without adding PBR exposure scaling.
- Apply pre-exposure once to PBR reflected and emissive radiance. Preserve the existing SimpleLit output scale, then use the common existing tonemap.
- Write one final HDR value per covered pixel. Clear HDR to the current background color, or fill background as part of classification, so uncovered pixels and empty scenes are deterministic.

The second classifier is deliberately loose tiled, as requested. DOOM's published deferred-lighting feature classification uses 32x32 tile masks; use its principle of small specialized shading kernels, but do not mistake that different scheduling scheme for this second pixel-compaction pass.

## 9. Core, frame graph and lifetime work

1. Add `DispatchIndirect` through public/common/Vulkan `FrameGraphContext`, using the existing indirect-argument binding and compute state setup. Indirect arguments contain three 32-bit group counts, with required offset alignment and bounds. Emit compute-write -> indirect-command-read synchronization. See the [Vulkan dispatch reference](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdDispatchIndirect.html).
2. Treat all cross-lane operations as baseline requirements on RDNA2+/Turing targets. Query wave lane counts and optional lane-count control, exposing `waveLaneCount` in the public compute API and its cache key. Do not equate `numthreads(64,1,1)` with guaranteed wave64. See the [Vulkan subgroup guide](https://docs.vulkan.org/guide/latest/subgroups.html).
3. RDNA2+ and Turing+ are the supported baseline. Assume R32G32_UINT visibility, R32_UINT GBuffer, sampled D32 depth and B10G11R11_UFLOAT HDR support. Query only wave size and wave-size control; do not add format support queries or HDR fallbacks.
4. Change depth to `D32_SFLOAT` and update material declarations/pipeline keys together. Commit it with both depth and shader-read usage, and use a depth-aspect view. Do not retain stencil solely to mark visibility validity.
5. Represent every stage with frame graph pass data and correct access declarations: visibility attachment writes -> classification reads; classification/sort writes -> scans/scatter; command writes -> indirect and shader reads; evaluation UAV writes -> shading classification/reads; depth writes -> compute reads; HDR writes -> tonemap reads.
6. Explicitly retain and declare GPU database pages, material allocation pages, device-address geometry and indirectly indexed textures. Resources discovered only through a shader's runtime pointer are not automatically visible to reflected pass descriptors.
7. Keep the first implementation on the existing graphics queue, which already runs compute. Async copy is not an async-compute scheduler. Queue overlap can be added only after ownership transfers, barriers and scratch lifetimes are implemented and useful overlapping work exists.
8. Allocate screen-sized resources per renderer/view policy, resize safely with frames in flight, and reset per-view counters/ranges. Preserve current sequential view/clear behavior; do not introduce accidental cross-view ID or scratch sharing.

Core's pool reuses completed resource instances; do not assume it provides general intra-frame aliasing of unrelated images. Explicit scratch-buffer reuse is immediate and useful. Image aliasing between visibility and later resources requires a separate compatible-memory/lifetime implementation and measurements before counting any savings.

## 10. Memory and bandwidth budget

For the initial required scenes, logical image residency before any aliasing is:

| Image | Bytes/pixel | 1920x1080 MiB | 3840x2160 MiB |
| --- | ---: | ---: | ---: |
| D32 depth | 4 | 7.91 | 31.64 |
| Visibility | 8 | 15.82 | 63.28 |
| GBuffer including emissive | 16 | 31.64 | 126.56 |
| Packed HDR | 4 | 7.91 | 31.64 |
| Total | 32 | 63.28 | 253.13 |

These figures exclude alignment/compression, swapchain, streamed assets, GPU database and full-view classification scratch. They are per image set, not a promise about total device allocation; account separately for resource generations retained by frames in flight.

The removed forward renderer used nominally 8-byte depth/stencil plus 4-byte HDR. This migration increases image residency; it aims to reduce redundant geometry work, material overshading and bandwidth relative to a naive deferred implementation, not to make visibility + GBuffer cheaper than the previous forward targets in every scene.

Priorities:

1. One geometry traversal; 8-byte visibility output; no full vertex attributes in the opaque visibility path.
2. No full-resolution world positions, barycentrics, derivatives, tangent frames, material IDs or duplicate PBR colors.
3. Read classification keys from the smallest plane available: 8-byte visibility initially, 4-byte G0 for the second stage, and G2 only when a real shading feature needs it.
4. Two-byte tile-local pixel lists with no stored lane padding; bounded metadata and sort scratch; reuse scratch between stages and windows.
5. Three base surface planes plus optional emissive, with unused planes untouched where safe. No non-PBR texture reads during final shading.
6. Store normals/material properties once and shade once per visible pixel. Coalesce coordinate packing and stores while retaining spatial locality.
7. Keep visibility alive only through material evaluation/debug consumers; GBuffer through shading/debug consumers. Shorten lifetimes now, add image aliasing only when supported.

Do not equate residency with bandwidth. At full coverage, base/extended GBuffer write + read alone can approach 32 bytes/pixel, and each complete visibility-classification scan adds 8 bytes/pixel before command traffic. Track scan count, sort traffic, geometry rereads, lane occupancy, command padding, register pressure, and stage GPU time. Tune tile-window size and fast histogram paths from those captures. Any later normal/parameter packing reduction must retain the helmet's specular quality; do not trade away precision solely to reach a round byte count.

## 11. Implementation stages

### Stage 1: Contracts and core support

- Capture reference frames for RendererSample's warm/textured/cool bunnies and helmet, and GameSample's current authored world, including streaming and camera motion.
- Establish visibility ID, surface layout, model IDs, uniform-group semantics and frame-lifetime contracts above.
- Add indirect compute dispatch, necessary capability handling, compute material roles/reflection, runtime registries and GPU routing schema. Keep the current renderer selectable while the new path is assembled.

Manual checkpoint: sample assets load with unchanged parameter values; registry/parameter-layout inspection and a GPU capture show valid compute pipelines, resource usages and indirect argument consumption.

### Stage 2: Depth/visibility rasterization and reconstruction

- Add minimal depth/visibility mesh and pixel shaders; reuse culling and indirect mesh draws.
- Wire the combined prepass and D32 depth into the frame graph.
- Add visibility/instance/meshlet/triangle debug views and temporary reconstructed UV/normal/derivative views.
- Implement attribute reconstruction, stable geometry lookup and explicit texture gradients before material shading depends on them.

Manual checkpoint: inspect silhouettes, occlusion, near-plane crossings, mirrored/nonuniform transforms, two-sided views and streaming LOD transitions. Compare reconstructed attributes to the old path, including mip selection during camera motion.

### Stage 3: First loose tiling and material GBuffer

- Implement the full-view count/prefix/scatter command builder, wave-width handling and race-free coordinate stores.
- Dispatch surface evaluation through GPU-written ranges, initially visualizing tile/group IDs and active lane counts.
- Port StandardPBR, Textured and both BunnyVariants permutations to the new surface contract; add encode/decode helpers and GBuffer debug views.
- Update and manually cook both sample depots so all required materials can populate the GBuffer.

Manual checkpoint: inspect every surface channel, material boundaries, normal-map orientation, palette/tint colors, UV scale, emissive values and streaming changes. Confirm high material diversity and partial tiles produce no missing, duplicate or stale pixels, and scratch follows the documented full-view capacity bound.

### Stage 4: Second loose tiling and deferred shading

- Reuse builder infrastructure with shading-model keys and separately generated pixel/wave/indirect commands.
- Add PBR and SimpleLit compute kernels, depth reconstruction, HDR writes and background handling.
- Connect existing tonemapping and compare old/new render paths with fixed view, exposure and asset residency.

Manual checkpoint: verify PBR highlights, brightness and emissive, all three bunny looks, and empty-scene/background output. Capture both stages to confirm material work and lighting work are separately classified, with no forward-only material fallback.

### Stage 5: Sample integration, residency and footprint tuning

- Make the new view pipeline the default in RendererSample and `WorldGraphicsSceneService`; update bridge draw tags and remove redundant opaque geometry submission.
- Complete reload/stream-out/fence retention, resize, multi-view scratch resets and unsupported-material diagnostics.
- Tune sparse classification, tile windows, command packing, optional G3 use and attribute fetches. Retain the old path only as long as needed for manual comparison, then remove obsolete roles/shaders/assets together.
- Update sample documentation and report measured stage times, peak image/scratch allocation and bandwidth observations at the same resolutions and views.

Manual checkpoint: use both applications interactively through camera/object motion, resize, loading transitions and material changes; inspect GPU captures for resource lifetime/synchronization and compare final appearance and memory cost to the recorded reference.

### Repository and asset workflow throughout

Add new C++ sources/headers to their nearest CMake lists and run `configure.bat` after adding/removing files. Use engine allocation/container patterns and format modified C++ with the bundled clang-format. Change reflected source/schema definitions, generate uppercase UUIDs for new reflected types, and run `scripts/codegen.py`; do not hand-edit generated GPU tables or reflection output.

Keep material definitions, shader dependencies, schema/version changes and cooked artifacts in sync. Manually rebuild material assets, material instances and any dependent source-less world assets with FerrumCli in dependency order, separately for Renderer and GameSample. Rebuild meshes only if their artifact layout actually changes; the proposed visibility encoding does not require it. Do not add asset cooking to CMake. GameSample's current authored world is the reference, rather than an assumed entity count from older descriptions.

## 12. Reference basis and deliberate adaptations

- [James McLaren, Adventures with Deferred Texturing in Horizon Forbidden West](../presentations/AdventuresWithDeferred_McLaren_James.pdf), PDF pages 31-36: large tiles, compact pixel commands, wave commands, uniform batch groups and shader-wide indirect dispatch. Pages 37-46 describe the bounded vertex ring strategy; pages 48-60 cover visibility and classification. Ferrum adopts loose tiling, but starts with inline static-mesh reconstruction rather than the presentation's animated-vertex ring pipeline.
- [Dominik Lazarek and Philip Hammer, Visibility Buffer and Deferred Rendering in DOOM: The Dark Ages](../presentations/Lazarek-Hammer-visibility-buffer-and-deferred-rendering-in-doom-the-dark-ages.pdf), pages 12-13: thin 64-bit visibility; pages 18-28: material-uniform compute commands, interpolation and deferred texturing; pages 30-37: separate deferred lighting and feature-specialized workloads; page 55: substantial resolution-dependent intermediate-memory cost. Ferrum uses 128x128 tiles and a second loose-tiling pass, rather than copying DOOM's 256x256 material tiles, separate full-screen interpolation buffers and 32x32 lighting tile masks.

The exact Ferrum ID encoding, GBuffer packing, bounded scratch strategy and material-role changes above are proposed adaptations based on the inspected repository, not formats claimed to come directly from either presentation.
