# GameSample

Build the `GameSample` and `FerrumCli` executables. GameSample owns its asset catalog with the same layout as Renderer:

- `Assets/source/Helmet/`: model, textures, and material source files, copied from Renderer.
- `Assets/Helmet/`: the corresponding authored `.asset` definitions.
- `Assets/World.asset`: the source-less two-entity world definition.
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
& $cli build --asset "$assets/World.asset" --output $assets
cmake-build/windows-debug-msvc/Debug/GameSample.exe
```

CMake builds executables only. The sample loads exclusively from its own `Assets` directory. `World.asset` is a normal source-less serialized asset with reflected component values. To create a new world asset, use `ferrum import --asset-type AB1B29E1-D167-43AA-8718-B3F8693FE184 --output <PATH>` and edit its authored values; reimport preserves those values and refreshes nested asset dependencies.

The disk world contains exactly two entities: Helmet (TransformComponent and MeshComponent) and Camera (TransformComponent and CameraComponent). GameFramework::Application initializes the renderer and asset streamers, then constructs the world's reflected, default-constructible services and systems. WorldGraphicsSceneService creates the scene, and the camera creates its View during component initialization. It does not construct entities or supply model/camera settings at runtime.

The shared application ticks assets, commits entity changes, evaluates transforms, extracts changed camera/mesh state on the main thread, then renders outside entity scheduling after the world update completes. Mesh and camera components own scene membership through non-serialized runtime fields; world teardown removes membership before releasing residency. No effects are included.

GameSample only supplies the world/catalog settings and selects normal or smoke execution. `GameSample.exe --smoke` renders eight frames and exits.
