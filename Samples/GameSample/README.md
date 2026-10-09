# GameSample

Build the `GameSample` target and run `cmake-build/windows-debug-msvc/Debug/GameSample.exe` from the repository root. The asset target copies Renderer's existing cooked dependencies into the build directory and cooks `Assets/World.asset` there.

The disk world contains exactly two entities: Helmet (TransformComponent and MeshComponent) and Camera (TransformComponent and CameraComponent). GameFramework::Application initializes the renderer and asset streamers, then constructs the world's reflected, default-constructible services and systems. WorldGraphicsSceneService creates the scene, and the camera creates its View during component initialization. It does not construct entities or supply model/camera settings at runtime.

The shared application ticks assets, commits entity changes, evaluates transforms, extracts changed camera/mesh state on the main thread, then renders outside entity scheduling after the world update completes. Transient companions own scene membership; world teardown removes membership before releasing residency. No effects are included.

GameSample only supplies the world/catalog settings and selects normal or smoke execution. `GameSample.exe --smoke` renders eight frames and exits. `GameSampleAssetBuilder.exe --author` recreates the authored fixture when component schemas change; normal builds only cook the saved source.
