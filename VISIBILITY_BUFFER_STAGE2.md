# Stage 2: coverage and reconstruction

The renderer supports combined reverse-Z D32 depth and `R32G32_UINT` visibility output, followed by compute reconstruction debug views. Forward rendering remains the default comparison path. Surface material evaluation and deferred lighting belong to stages 3 and 4.

## Selecting a view

Both RendererSample and GameSample accept `--visibility-debug <mode>`. Modes are `forward`, `visibility`, `instance`, `meshlet`, `triangle`, `uv`, `normal`, `tangent`, `derivatives`, and `depth-error`. Add `--smoke` to render eight frames and exit. Applications can call `View::SetVisibilityDebugMode`; GameFramework's application settings select the mode applied to its camera views.

ID views color the complete visibility pair, persistent instance, meshlet, or local triangle. UV displays wrapped source UVs. Normal and tangent display transformed directions. Derivatives show absolute UV X derivatives in red/green and UV Y derivative length in blue, scaled by 32. Depth error shows the absolute difference between reconstructed and raster depth, scaled by 100,000. Invalid reconstruction is magenta. Diagnostics currently pass through the existing ACES presentation path.

## Geometry contract

`DepthVisibility` uses existing amplification culling, a position-only mesh shader, the visibility pixel shader, greater/write D32 depth and one integer attachment. Textured, both BunnyVariants palettes and StandardPBR declare it with their existing cull permutations. The opaque draw tag selects coverage; this path omits the old depth prepass and opaque pass. All sample depth/opaque techniques also use D32 to preserve the forward comparison path.

Mesh outputs carry the persistent database instance row plus one and the actual amplification-payload meshlet index plus local primitive index. IDs are per-primitive outputs; triangle winding is unchanged. The bundled DXC needs explicit SPIR-V decoration attributes on pixel inputs to match that interface ([DXC issue 6862](https://github.com/microsoft/DirectXShaderCompiler/issues/6862)). Vulkan pipeline creation and dynamic rendering both attach depth/stencil aspects only when present.

`MeshPass/Geometry.hlsli` shares geometry lookup and transformed attributes between forward and reconstruction paths. Reconstruction resolves instance → group → the group's resident LOD, then meshlet header → packed triangle → local vertex remap → packed vertex. It reuses device addresses without additional cooked triangle buffers. Scene/database updates happen once before view processing. The database transitions uploaded dirty pages to shader-readable state; clean pages retain their established readable state from previous frames. Scene geometry declarations retain geometry through submission. Work uses the existing graphics queue. Async scheduling and independent per-view LOD selection must extend this contract later.

## Interpolation and gradients

Reconstruction solves perspective weights directly in homogeneous clip space at each pixel center. It avoids dividing source vertices by W, including vertices on or behind the eye plane. Degenerate/nonfinite solves fail explicitly. Normals use the same inverse-transpose transform as forward shading; tangent transforms and mirrored handedness match too. Two-sided rasterization does not implicitly flip normals.

The solve differentiates its rational weights analytically in framebuffer pixel units. `SampleReconstructedTexture` applies UV scale to UV and both gradients and calls `SampleGrad`. Later material ports will consume this sampling helper; this stage does not port their texture equations or GBuffer encodings. Debug UVs are source UVs, without material UV scale.

Visibility receives an integer clear and D32 receives reverse-Z zero. The HDR debug dispatch covers the complete target, including background and partial workgroups. Visibility resizes with the viewport; each view gets its own clear/coverage/reconstruction sequence. Stage 2 diagnostics require packed HDR storage writes, matching the existing forward color format; unsupported requests report a diagnostic.

## Validation checkpoint

Validated on 2026-10-14: all seven graphics tests, all 24 GameFramework tests and the standalone reflection-link check pass. RendererSample smoke runs pass for forward, visibility, UV, derivatives and depth error; GameSample passes forward, normal and depth error. GPU validation reports no errors in these runs; the existing configuration/unused-output warnings remain. Material assets and dependent instances were manually recooked in all three sample depots.

The graphics GPU test includes perspective/derivative probes with zero and negative source W, a degenerate triangle, and finite-difference derivative comparisons. A raster/readback probe exercises the actual amplification/mesh/pixel pipeline, shuffled remap indices, nonzero meshlet/primitive offsets and persistent instance rows, mirrored nonuniform transforms, integer clears, D32 depth, UVs, normals, tangent handedness and UV derivatives.

Manual review remains: inspect silhouettes, occlusion and coplanar ownership, near-plane clipping, two-sided views, streaming LOD transitions, camera motion and resize. Compare diagnostics against fixed forward references and inspect GPU captures for barriers, geometry addresses and IDs. Automated readback does not replace these checkpoints.
