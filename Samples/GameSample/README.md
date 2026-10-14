# GameSample

Stage 3 adds `evaluation-tile`, `evaluation-group`, `surface-color`, `surface-normal`, `surface-properties`, `surface-emissive`, and `surface-route` modes to `--visibility-debug`. These run GPU loose tiling and surface evaluation; lighting remains forward until Stage 4. See [Stage 3 notes](../../VISIBILITY_BUFFER_STAGE3.md).

Use `--visibility-debug <mode>` for Stage 2 diagnostics (`visibility`, `instance`, `meshlet`, `triangle`, `uv`, `normal`, `tangent`, `derivatives`, `depth-error`); the default is forward shading. Add `--smoke` to render eight frames after world readiness and exit. See [Stage 2 notes](../../VISIBILITY_BUFFER_STAGE2.md).

Build the `GameSample` and `FerrumCli` executables. GameSample owns its asset catalog with the same layout as Renderer:

- `Assets/source/Helmet/`: model, textures, and material source files, copied from Renderer.
- `Assets/Helmet/`: the corresponding authored `.asset` definitions.
- `Assets/World.asset`: the source-less world definition with 100 helmets and one camera.
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
& $cli build --asset "$assets/World.asset" --output $assets
cmake-build/windows-debug-msvc/Debug/GameSample.exe
```

CMake builds executables only. The sample loads exclusively from its own `Assets` directory. `World.asset` is a normal source-less serialized asset with reflected component values. To create a new world asset, use `ferrum import --asset-type AB1B29E1-D167-43AA-8718-B3F8693FE184 --output <PATH>` and edit its authored values; reimport preserves those values and refreshes nested asset dependencies.

The disk world contains 100 helmets (TransformComponent and MeshComponent) in a 10 by 10 grid with three-unit spacing, plus one camera (TransformComponent, CameraComponent and CameraMovementComponent). The movement component references its mapping table and three action assets through authored `IO::Link` fields. GameFramework::Application initializes the renderer and asset streamers, then constructs the world's reflected, default-constructible services and systems. WorldGraphicsSceneService creates the scene, and the camera creates its View during component initialization. It does not construct entities or supply model/camera settings at runtime.

Move with WASD or arrows, elevate with E/Q, and hold RMB while moving the mouse to rotate. Movement uses elapsed world time; mouse rotation uses pixel deltas. The world registers WorldTimeService before WorldInputService and CameraMovementSystem before transform propagation. Cameras sharing a mapping table use one stack resolution per frame. Change action links or key/mouse bindings in the authored assets and rebuild them to customize controls.

The shared application ticks assets, commits entity changes, evaluates transforms, extracts changed camera/mesh state on the main thread, then renders outside entity scheduling after the world update completes. Mesh and camera components own scene membership through non-serialized runtime fields; world teardown removes membership before releasing residency. TestRotationSystem rotates every mesh entity around its local parent Y axis at one radian per second before transform propagation. No effects are included.

GameSample only supplies the world/catalog settings and selects normal or smoke execution. `GameSample.exe --smoke` renders eight frames after world readiness and exits. World loading advances during normal frames; until a camera is ready, the renderer presents a cleared background.

Input contexts are authored in the input service descriptor in `World.asset`. Cameras reference movement, elevation, and look action assets; RMB look sensitivity is authored in the mouse-delta binding.
