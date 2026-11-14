# GameSample

The sample is split into `GameSample_Lib` (all game and game-specific editor code),
`GameSample` (the game launcher), and `GameSample_Editor` (the editor launcher).
Each launcher has exactly one `main.cpp`; `fe_add_game_project()` creates all three targets
in one call. Both executables use the existing `Assets` catalog.

Run `cmake-build/windows-debug-msvc/Debug/GameSample_Editor.exe` for the paused editor.
Its dock layout provides an World and Entity windows with RTTI component inspection and transform
controls, an asset grid, and a scene viewport. Double-click a world asset or use File > Open
(Ctrl+O) to load it. Click geometry to select, drag axis handles to translate, and hold RMB
with WASD/QE to fly the editor camera (Shift speeds up movement). The camera is transient
and the authored world remains paused without physics simulation.

`GameSample_Editor.exe --smoke` renders eight ready frames and exercises selection
outline and pixel picking. The regular `GameSample.exe --smoke` remains available.

Visibility rasterization, GBuffer evaluation and deferred compute shading are the default pipeline. Each stage has its own view module and exchanges targets through the FrameGraph blackboard. Transparent materials use a clustered forward pass after deferred lighting and before tonemapping. See [Stage 5 notes](../../VISIBILITY_BUFFER_STAGE5.md).

Validation is disabled by default. Add `--ecs-validation` to check ECS schedules. Add `--api-validation` for Vulkan API checks, or `--gpu-validation` for GPU-assisted checks (which also enables API validation). Leave validation disabled for performance measurements and Nsight captures. The former `--no-api-validation` flag only hid validation messages and did not disable validation.

With `TRACY_ENABLE`, Tracy shows `Vulkan Graphics` and `Vulkan Async Copy` GPU contexts. Each queue has one GPU track, independent of the CPU worker recording its commands. FrameGraph scopes and passes retain their names; copy batches contain buffer-copy, buffer-upload and texture-upload zones. Transfer-only queues require timestamp support and host query resets. During continuous uploads, timing zones are skipped if the query pool fills and resume after completed submissions are collected, without blocking uploads.

Stage 3 adds `evaluation-tile`, `evaluation-group`, `surface-color`, `surface-normal`, `surface-properties`, `surface-emissive`, and `surface-route` modes to `--visibility-debug`. These show GPU classification and surface evaluation. See [Stage 3 notes](../../VISIBILITY_BUFFER_STAGE3.md).

Use `--visibility-debug <mode>` for Stage 2 diagnostics (`visibility`, `instance`, `meshlet`, `triangle`, `uv`, `normal`, `tangent`, `derivatives`, `depth-error`). Add `--smoke` to render eight frames after world readiness and exit. See [Stage 2 notes](../../VISIBILITY_BUFFER_STAGE2.md).

Camera views enable FSR3 upscaling at Quality (`Fsr3::ViewModule::m_renderScale = 2.0f / 3.0f`). Set the scale to `1.0f`
for Native AA, or deactivate the module for native rendering. Debug visualizations bypass upscaling. Transparent materials
write an additional `R8_UNORM` reactive render target with max blending; `Fsr3::Masks` accepts a separate optional
transparency/composition texture for effects that need it. The upscaler runs before tonemapping and UI, without frame generation.

Build the `GameSample` and `FerrumCli` executables. GameSample owns its asset catalog with the same layout as Renderer:

- `Assets/source/Helmet/`: model, textures, and material source files, copied from Renderer.
- `Assets/Helmet/`: the corresponding authored `.asset` definitions.
- `Assets/World.asset`: the source-less world definition with 50 dynamic rigid bodies in five layers, a static box floor and one camera.
- `Assets/source/`: bunny model, texture and material scripts copied from Renderer.
- `Assets/*.asset`: bunny model, texture and three material variants.
- `Assets/Input/`: source-less movement, elevation, look actions and the world input mapping context.
- `Assets/artifacts/`: cooked data and metadata produced by FerrumCli, including the engine's shader packages and editor
  input assets, whose definitions live in `Modules/Graphics/Framework/Assets` and `Modules/GameFramework/Assets`.

The catalog is built from three depots: the sample's own and the two engine module depots. From the repository root:

```powershell
$cli = "cmake-build/windows-debug-msvc/Debug/ferrum.exe"
$depots = "Samples/GameSample/Assets;Modules/GameFramework/Assets;Modules/Graphics/Framework/Assets"
& $cli build --all --depot $depots --output Samples/GameSample/Assets
cmake-build/windows-debug-msvc/Debug/GameSample.exe
```

`build --all` finds every `.asset` file, builds definitions in dependency order and runs independent builds in parallel.
Products whose inputs did not change are skipped. While editing assets, run `& $cli serve --depot $depots --output
Samples/GameSample/Assets` instead: it rebuilds whatever a change affects, and a running game or editor reloads the new
artifacts. `& $cli status --output Samples/GameSample/Assets` prints the server's build status and errors. Build results
are kept in `Assets/.ferrum-build`, which is not committed; builds never modify `.asset` files.

New definitions are still created with `import`, e.g. `& $cli import --asset Samples/GameSample/Assets/source/Floor.gltf`
or `& $cli import --asset-type AB1B29E1-D167-43AA-8718-B3F8693FE184 --output Samples/GameSample/Assets/World.asset`.

CMake builds executables only. The sample loads exclusively from its own `Assets` directory. `World.asset` is a normal source-less serialized asset with reflected component values. To create a new world asset, use `ferrum import --asset-type AB1B29E1-D167-43AA-8718-B3F8693FE184 --output <PATH>` and edit its authored values; reimport preserves those values and refreshes nested asset dependencies.

The disk world contains 50 helmets and bunnies in five layers of ten (a 5 by 2 grid per layer, with nominal 3-meter horizontal and 2.6-meter vertical spacing, randomized by up to 0.3 meters horizontally and 0.08 meters vertically), each with TransformComponent, MeshComponent, and a dynamic PhysicsBodyComponent. All 50 use the shared Sphere.asset collision asset. A visible cube mesh with a static box collider forms the floor. The world registers WorldPhysicsService and PhysicsSystem; simulation runs at 60 Hz before transform propagation and graphics extraction. The camera retains authored movement controls.

Move with WASD or arrows, elevate with E/Q, and hold RMB while moving the mouse to rotate. Movement uses elapsed world time; mouse rotation uses pixel deltas. The world registers WorldTimeService before WorldInputService and CameraMovementSystem before transform propagation. Cameras sharing a mapping table use one stack resolution per frame. Change action links or key/mouse bindings in the authored assets and rebuild them to customize controls.

The shared application ticks assets, commits entity changes, evaluates transforms, extracts camera and mesh membership on the main thread, extracts mesh transforms in parallel, then renders outside entity scheduling after the world update completes. Mesh and camera components own scene membership through non-serialized runtime fields; world teardown removes membership before releasing residency. PhysicsSystem advances rigid bodies during fixed updates. No effects are included.

GameSample only supplies the world/catalog settings and selects normal or smoke execution. `GameSample.exe --smoke` renders eight frames after world readiness and exits. World loading advances during normal frames; until a camera is ready, the renderer presents a cleared background.

Application opens a shared materialization budget once per rendered frame: 32 construction/loading entity steps and 32 entities admitted to initialization/activation. Large worlds therefore become ready across several frames. Repeated commits or fixed updates in that frame share the quota. Octree culling combines small subtrees into batches targeting 64 entries; the profiler zone text reports the number of ranges and entries in each batch.

Input contexts are authored in the input service descriptor in `World.asset`. Cameras reference movement, elevation, and look action assets; RMB look sensitivity is authored in the mouse-delta binding.

Global lighting is also authored in `World.asset`, in the `WorldGraphicsSceneService` configuration. It specifies the sun's
orientation, linear RGB tint and reference daylight illuminance in lux, plus Hosek sky turbidity (1-10), ground albedo and
sun angular radius in radians. Local +Z of the sun orientation points towards the sun; world +Y points up. At 120000 lux the
model uses its physical daylight scale. Lower values proportionally dim both the sun and sky.

The graphics service owns `GlobalLightingSceneModule`, which precomputes RGB sky coefficients and integrates the spectral
solar disc on the CPU when lighting settings change. The solar spectrum is converted through CIE XYZ to linear RGB; the
resulting attenuated sunlight drives both PBR and simple lit materials. Background pixels evaluate the
[Hosek-Wilkie model](https://github.com/ebruneton/clear-sky-models/tree/master/atmosphere/model/hosek) and a uniform sun disc.
Changing the displayed sun radius preserves its integrated illuminance. The lower hemisphere uses Lambertian ground
radiance from CPU-integrated sky and sunlight. The sun and sky switch off below the horizon; night skies, altitude-dependent
lighting, indirect skylight on geometry and aerial perspective are not implemented. Exposure and pre-exposure remain fixed
at EV100 12. Clear colors are black.

The editor's **Services** window lists registered world services and their captured configuration. Lighting settings, including
the nested atmosphere parameters, can be changed live even while paused; invalid values are rejected. Settings for services
without a runtime configuration API are read-only. Live edits last for the current world session; author persistent values in
the world asset and rebuild it with FerrumCli.

## Debug UI

The "Culling" section toggles instance frustum and occlusion culling, meshlet frustum and occlusion culling, and LOD selection, sets the LOD error threshold in pixels, and freezes the culling camera. While frozen, all culling (frustum, LOD selection, instance and meshlet occlusion against the depth pyramid captured when the freeze started) uses the camera of that frame, and its frustum is drawn in yellow, so moving away shows exactly what that camera would render. "Culling statistics" shows the first view's instances, meshlets and triangles tested, culled and rendered, and rendered instances per LOD. "Color meshes by LOD" (debug mode `lod`) colors meshes by their selected LOD, from green (most detailed) through yellow and red to blue. `--no-occlusion-culling` and `--no-lod-selection` set the initial settings; smoke runs log the statistics.

The sample draws a Dear ImGui "Rendering" window. Its debug mode list is built from the modes published by the camera view's modules (`Visibility::DebugViewModule` for reconstruction modes, `GBufferDebug::ViewModule` for surface modes); the selection applies to every camera view at runtime. `--visibility-debug <mode>` selects the initial mode (`deferred` keeps lit output). Docking and multi-viewports are enabled: dragging a window outside the main window gives it its own platform window and swapchain. `--detach-debug-window` places the window outside the main viewport, which exercises that path in smoke runs. Window layout persists in `imgui.ini` next to the sample's build directory.

The editor's input assets are defined once in `Modules/GameFramework/Assets` and cooked into `Assets` by the depot build
above, so the game and the editor use the same catalog. Gameplay input assets remain under `Assets/Input`.

Shader sources come from the module directories during development. `--shader-packages` makes the sample read them only
from the shader packages in its catalog, as a build without the source tree does.


## Local lights and transparency

`LocalLightingSceneModule` builds a view-local 32x32 pixel, 24 logarithmic depth-slice grid on the GPU. Conservative sphere/AABB
and cone/sphere tests produce dynamically sized light bitmasks, shared by deferred and forward PBR shading through `ComputeShading`. SimpleLit uses directional lighting only.
The shader scalarizes bitmask iteration across each wave. The grid covers the entire view frustum, including empty opaque
pixels, so transparent surfaces can look up lights at their own depth. It follows the cluster and flat-bitmask approaches in
Drobot's 2017 Improved Culling presentation and the Doom Eternal 2020 rendering presentation. Hybrid depth-refined tile
lists and hash scalarization from Doom Eternal are future optimizations.

`PointLightComponent` and `SpotLightComponent` own generation-checked scene slots allocated on activation and released on
deactivation. `LocalLightingSceneModule` stores their data in persistent `LocalLightTable` GPU database rows. `LightSystem`
extracts changed component parameters or world transforms after transform propagation; unchanged values never dirty a row.
Each light occupies 32 bytes: full-precision position/range, R11G11B10 color with a float intensity multiplier,
octahedral SNORM16x2 direction, and two UNORM16 half-angles (with a separate point-light sentinel). Packing angles avoids
loss of narrow spot cones near cosine 1. The database uploads only dirty pages through the same path as meshes. Freed rows are disabled before slot reuse, and
cluster masks index stable row slots. Only view-dependent cluster constants and masks are rebuilt each frame.
Both expose `LightUnit` (candela or lumen), `LightColorMode` (linear RGB or temperature), intensity, range in meters and
color temperature in kelvin (1000–40000 K). Temperature integrates a blackbody spectrum against the same CIE matching
functions as the sky and normalizes luminance. Point lumen values divide by 4 pi; spot lumen values integrate the smooth
inner/outer cone profile. Spot angles are half-angles in radians and local +Z is the emission axis. Transform scale does
not change light range. Illumination uses inverse-square falloff with a smooth finite-range window and the existing
pre-exposure. All three lights set `m_castsShadows`, which opts a light into virtual shadow maps: a spot light takes one
perspective map and a point light six cube faces, each with four mips, in the page table and atlas of the sun shadows.
A camera view shadows at most 16 maps, chosen by screen coverage. The `local-shadow` debug mode shows their visibility.

Materials with a `ForwardTransparency` technique select `DrawTags::Transparent`; opaque meshes select `DrawTags::Visibility`.
Transparent objects are culled separately against the opaque depth pyramid and do not write opaque occlusion history or
depth. Their `ShadowPageMark` technique requests the shadow pages they sample, so they receive sun and local shadows.
`TransparentPBR.mat.luau` demonstrates independent target blends, a constant base color/opacity and roughness/metallic PBR.
Six bunnies use this material. The world also contains two colored point lights and one spotlight.

The forward pass accumulates weighted premultiplied lighting and multiplicative revealage in separate targets, then
composites onto HDR color. This weighted blended order-independent transparency avoids object sorting and handles
intersecting meshes, but approximates layered alpha compositing; refraction, transmission and shadows cast by
transparent objects are not implemented. Transparent objects are excluded from the opaque visibility/GBuffer debug views.
