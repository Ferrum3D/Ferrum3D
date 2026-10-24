# Renderer sample

Visibility rasterization, GBuffer evaluation and deferred compute shading are the default pipeline. Each stage has its own view module and exchanges targets through the FrameGraph blackboard. The forward path has been removed. See [Stage 5 notes](../../VISIBILITY_BUFFER_STAGE5.md).

Validation is disabled by default. Add `--api-validation` for Vulkan API checks, or `--gpu-validation` for GPU-assisted checks (which also enables API validation). Use neither flag for performance measurements and Nsight captures.

Stage 3 adds `evaluation-tile`, `evaluation-group`, `surface-color`, `surface-normal`, `surface-properties`, `surface-emissive`, and `surface-route` modes to `--visibility-debug`. These show GPU classification and surface evaluation. See [Stage 3 notes](../../VISIBILITY_BUFFER_STAGE3.md).

Use `--visibility-debug <mode>` for Stage 2 diagnostics (`visibility`, `instance`, `meshlet`, `triangle`, `uv`, `normal`, `tangent`, `derivatives`, `depth-error`). `--smoke` renders eight frames and exits. See [Stage 2 notes](../../VISIBILITY_BUFFER_STAGE2.md).

Build `RendererSample` and launch it from `cmake-build/windows-debug-msvc/Samples/Renderer`.
The sample displays warm bunnies on the left, textured bunnies and a damaged helmet in the center, and cool bunnies on the right,
using the visibility and deferred passes. Every object rotates around its local Y axis at 30 degrees per second,
with frame delta time measured by the engine high-resolution timer.

GPU validation, lifecycle stress modes, and benchmarks live in `Samples/GpuDrivenTestApp`.
Each application has its own asset depot. Renderer uses `StanfordBunny`, `Textured`, `texture`, `BunnyMaterial`, `BunnyVariants`, `BunnyWarm`, and `BunnyCool`.
Rebuild these with FerrumCli using `Samples/Renderer/Assets/source` as the source root and `Samples/Renderer/Assets` as the output root.

The helmet uses explicit StandardPBR material and base-color, ORM, BC5 normal, and emissive DDS texture assets.
Both meshes request LOD 0 for full detail. Mesh requests use finest-first indices, like texture mip requests;
the streamer converts them to the internal coarsest-first storage order.
After initial asset loading, requested mesh LODs and texture mips stream in the background while the sample renders.
Base color and emissive are sampled through sRGB formats; ORM and normals remain linear.
The source normal DDS uses the glTF/MikkTSpace +Y convention and is imported without conversion.
The PBR shader uses GGX, height-correlated Smith visibility, Schlick Fresnel, and energy-conserving Lambert diffuse.
`GlobalLightingSceneModule` supplies a sun with 10,000 lux perpendicular illuminance before atmospheric attenuation,
and a spherical Rayleigh/Mie/ozone sky atmosphere. The sample configures this module explicitly; GameSample authors the same
settings in its world asset. Emissive texels modulate 1,000 cd/m^2, and fixed EV100 12 pre-exposure is applied before writing HDR.
The sky and sun disc fill background pixels in HDR before tonemapping. There is no IBL; AO is reserved for future indirect lighting.
The final pass uses the Narkowicz ACES filmic fit followed by the sRGB transfer function into a UNORM swapchain
with the sRGB nonlinear presentation color space.

After building FerrumCli, build these files in order using `ferrum build --asset <file>
--source-root Samples/Renderer/Assets/source --output Samples/Renderer/Assets`:
`Helmet/DamagedHelmet.asset`, `Helmet/StandardPBR.asset`, `Helmet/DamagedHelmet_a.asset`,
`Helmet/DamagedHelmet_orm.asset`, `Helmet/DamagedHelmet_n.asset`, `Helmet/DamagedHelmet_e.asset`,
and `Helmet/DamagedHelmetMaterial.asset` (all paths under `Samples/Renderer/Assets`). Model import generates missing tangents
with meshoptimizer's MikkTSpace-compatible mode and preserves tangent seams.
