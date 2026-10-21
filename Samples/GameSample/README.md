# GameSample

Visibility rasterization, GBuffer evaluation and deferred compute shading are the default pipeline. Each stage has its own view module and exchanges targets through the FrameGraph blackboard. The forward path has been removed. See [Stage 5 notes](../../VISIBILITY_BUFFER_STAGE5.md).

Validation is disabled by default. Add `--ecs-validation` to check ECS schedules. Add `--api-validation` for Vulkan API checks, or `--gpu-validation` for GPU-assisted checks (which also enables API validation). Leave validation disabled for performance measurements and Nsight captures. The former `--no-api-validation` flag only hid validation messages and did not disable validation.

With `TRACY_ENABLE`, Tracy shows `Vulkan Graphics` and `Vulkan Async Copy` GPU contexts. Each queue has one GPU track, independent of the CPU worker recording its commands. FrameGraph scopes and passes retain their names; copy batches contain buffer-copy, buffer-upload and texture-upload zones. Transfer-only queues require timestamp support and host query resets. During continuous uploads, timing zones are skipped if the query pool fills and resume after completed submissions are collected, without blocking uploads.

Stage 3 adds `evaluation-tile`, `evaluation-group`, `surface-color`, `surface-normal`, `surface-properties`, `surface-emissive`, and `surface-route` modes to `--visibility-debug`. These show GPU classification and surface evaluation. See [Stage 3 notes](../../VISIBILITY_BUFFER_STAGE3.md).

Use `--visibility-debug <mode>` for Stage 2 diagnostics (`visibility`, `instance`, `meshlet`, `triangle`, `uv`, `normal`, `tangent`, `derivatives`, `depth-error`). Add `--smoke` to render eight frames after world readiness and exit. See [Stage 2 notes](../../VISIBILITY_BUFFER_STAGE2.md).

Build the `GameSample` and `FerrumCli` executables. GameSample owns its asset catalog with the same layout as Renderer:

- `Assets/source/Helmet/`: model, textures, and material source files, copied from Renderer.
- `Assets/Helmet/`: the corresponding authored `.asset` definitions.
- `Assets/World.asset`: the source-less world definition with 50 dynamic rigid bodies in five layers, a static box floor and one camera.
- `Assets/source/`: bunny model, texture and material scripts copied from Renderer.
- `Assets/*.asset`: bunny model, texture and three material variants.
- `Assets/Input/`: source-less movement, elevation, look actions and the world input mapping context.
- `Assets/artifacts/`: cooked data and metadata produced by FerrumCli.

Build the assets manually from the repository root, in dependency order:

```powershell
$cli = "cmake-build/windows-debug-msvc/Debug/ferrum.exe"
$assets = "Samples/GameSample/Assets"
& $cli build --asset "$assets/Helmet/DamagedHelmet.asset" --source-root "$assets/source" --output $assets
& $cli build --asset "$assets/Helmet/StandardPBR.asset" --source-root "$assets/source" --output $assets
& $cli build --asset "$assets/Helmet/DamagedHelmet_a.asset" --source-root "$assets/source" --output $assets
& $cli build --asset "$assets/Helmet/DamagedHelmet_e.asset" --source-root "$assets/source" --output $assets
& $cli build --asset "$assets/Helmet/DamagedHelmet_n.asset" --source-root "$assets/source" --output $assets
& $cli build --asset "$assets/Helmet/DamagedHelmet_orm.asset" --source-root "$assets/source" --output $assets
& $cli build --asset "$assets/Helmet/DamagedHelmetMaterial.asset" --output $assets
& $cli build --asset "$assets/Input/Move.asset" --output $assets
& $cli build --asset "$assets/Input/Elevate.asset" --output $assets
& $cli build --asset "$assets/Input/Look.asset" --output $assets
& $cli build --asset "$assets/Input/CameraMappings.asset" --output $assets
& $cli build --asset "$assets/StanfordBunny.asset" --source-root "$assets/source" --output $assets
& $cli build --asset "$assets/Textured.asset" --source-root "$assets/source" --output $assets
& $cli build --asset "$assets/texture.asset" --source-root "$assets/source" --output $assets
& $cli build --asset "$assets/BunnyVariants.asset" --source-root "$assets/source" --output $assets
& $cli build --asset "$assets/BunnyMaterial.asset" --output $assets
& $cli build --asset "$assets/BunnyWarm.asset" --output $assets
& $cli build --asset "$assets/BunnyCool.asset" --output $assets
& $cli import --asset "$assets/source/Floor.gltf" --output "$assets/Floor.asset"
& $cli build --asset "$assets/Floor.asset" --source-root "$assets/source" --output $assets
& $cli import --asset-type 3A4A6717-42D0-4393-9C32-A84949024167 --output "$assets/Sphere.asset"
& $cli build --asset "$assets/Sphere.asset" --output $assets
& $cli import --asset-type AB1B29E1-D167-43AA-8718-B3F8693FE184 --output "$assets/World.asset"
& $cli build --asset "$assets/World.asset" --output $assets
cmake-build/windows-debug-msvc/Debug/GameSample.exe
```

CMake builds executables only. The sample loads exclusively from its own `Assets` directory. `World.asset` is a normal source-less serialized asset with reflected component values. To create a new world asset, use `ferrum import --asset-type AB1B29E1-D167-43AA-8718-B3F8693FE184 --output <PATH>` and edit its authored values; reimport preserves those values and refreshes nested asset dependencies.

The disk world contains 50 helmets and bunnies in five layers of ten (a 5 by 2 grid per layer, with nominal 3-meter horizontal and 2.6-meter vertical spacing, randomized by up to 0.3 meters horizontally and 0.08 meters vertically), each with TransformComponent, MeshComponent, and a dynamic PhysicsBodyComponent. All 50 use the shared Sphere.asset collision asset. A visible cube mesh with a static box collider forms the floor. The world registers WorldPhysicsService and PhysicsSystem; simulation runs at 60 Hz before transform propagation and graphics extraction. The camera retains authored movement controls.

Move with WASD or arrows, elevate with E/Q, and hold RMB while moving the mouse to rotate. Movement uses elapsed world time; mouse rotation uses pixel deltas. The world registers WorldTimeService before WorldInputService and CameraMovementSystem before transform propagation. Cameras sharing a mapping table use one stack resolution per frame. Change action links or key/mouse bindings in the authored assets and rebuild them to customize controls.

The shared application ticks assets, commits entity changes, evaluates transforms, extracts camera and mesh membership on the main thread, extracts mesh transforms in parallel, then renders outside entity scheduling after the world update completes. Mesh and camera components own scene membership through non-serialized runtime fields; world teardown removes membership before releasing residency. PhysicsSystem advances rigid bodies during fixed updates; TestRotationSystem is not registered. No effects are included.

GameSample only supplies the world/catalog settings and selects normal or smoke execution. `GameSample.exe --smoke` renders eight frames after world readiness and exits. World loading advances during normal frames; until a camera is ready, the renderer presents a cleared background.

Application opens a shared materialization budget once per rendered frame: 32 construction/loading entity steps and 32 entities admitted to initialization/activation. Large worlds therefore become ready across several frames. Repeated commits or fixed updates in that frame share the quota. Octree culling combines small subtrees into batches targeting 64 entries; the profiler zone text reports the number of ranges and entries in each batch.

Input contexts are authored in the input service descriptor in `World.asset`. Cameras reference movement, elevation, and look action assets; RMB look sensitivity is authored in the mouse-delta binding.
