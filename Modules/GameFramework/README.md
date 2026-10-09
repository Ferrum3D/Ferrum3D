# GameFramework

`FeGameFramework` is the separate engine-system project. It depends on `FeFramework`, which retains the reusable entity runtime and its GPU-independent tests. GameFramework owns transformation, graphics and streaming components and systems; other engine systems belong here as well. It also depends on Graphics for scene integration.

## Stage 7 implementation

- Reflected, serialized `TransformComponent` stores an authoritative `FE::Transform` (translation, quaternion rotation, and uniform scale).
- Optional authored `NonUniformScaleComponent` supplies a scale modifier.
- `WorldTransformComponent` stores transient computed output. TransformationSystem registers its transient policy before entities are created.
- `TransformationSystem` uses changed cascade queries and parallel tree batches. Unchanged epochs skip transform callbacks. Parent/local/scale changes, new children and hierarchy changes conservatively propagate through matching descendants.

The engine uses row vectors. Composition is `Scale(modifier) * Transform::ToMatrix(local) * parentWorld`; a missing immediate parent WorldTransform contributes identity. A node without WorldTransform does not transmit a more distant ancestor transform. Entities participate only when they have both local Transform and WorldTransform components.

```cpp
GameFramework::TransformationSystem transforms;
Framework::EntityWorld world;
world.AddSystem(transforms); // The system must outlive registration.

Framework::EntityCommandList commands(world);
auto root = commands.CreateEntity(world.CreateRegistry(), "root");
commands.AddComponent(root, GameFramework::TransformComponent{ Transform::Identity() });
commands.AddComponent<GameFramework::WorldTransformComponent>(root);
world.Submit(std::move(commands));
world.CommitBootstrap();

world.BeginUpdate();
world.SchedulePhase(GameFramework::Phases::Transformation);
world.ExecuteSchedule();
world.EndUpdate();
world.RemoveSystem(transforms);
```

Application code schedules Transformation in its desired phase order. `GetCompletion()` provides the epoch's completion for explicitly ordered dependent work. Engine-system tests live in `FeGameFrameworkTests`; scheduler, lifecycle, residency and generic change-tracking tests remain in `FeFrameworkTests`.

`EntityCommandList::SetParent` defaults to `ReparentMode::kPreserveWorld`. TransformationSystem installs the generic Framework reparent handler during Init and clears it during Shutdown. The handler evaluates authored ancestor chains against the transactional command view, including preceding replacements and reparents, so it does not depend on cached output freshness. It computes an affine inverse and decomposes the new local matrix into `FE::Transform` and the existing optional scale modifier. Results requiring shear or non-uniform scale without a modifier, failed decompositions, and invalid/non-invertible parents reject the entire command list before changing live components or links. Use explicit `kPreserveLocal` when constructing an authored hierarchy whose local values already describe its parent relationship.

Commands and collection materialization automatically create transient WorldTransformComponent output for authored TransformComponent through a generic runtime-companion registration. Placement roots store their authored transform in the placement asset's generic root component envelope; Framework contains no engine transform types. Cook it with `placement.m_root.CookComponent(placement.m_root.m_entities.front(), TransformComponent{ placementTransform })` after `UpdateBindings(collection)`.

Changing local storage from a matrix to `FE::Transform` changes the generated serialization schema. Rebuild cooked assets containing TransformComponent.


CameraSystem and MeshSystem register authored components and transient graphics companions. WorldGraphicsSceneService creates and owns each world's scene and mesh bridge. Applications order Transformation before GraphicsExtraction and render after the world update completes. Changed queries collect entity IDs; main-thread stages update View matrices and MeshSceneModule membership without retaining component addresses in the renderer. Invalid camera updates leave the previous matrices usable. Each camera creates its own View during component initialization, enables it during activation, and removes it during shutdown.

WorldStreamingService queues placement loads and registry unloads by persistent registry key. Its service update applies requests before structural commit; pending unloads cancel materialization and residency. GetStatus reports pending, ready, failed, or canceled operations. Registries do not split simulation queries or scheduling.

GameSample builds and loads a cooked world with one Helmet mesh and one camera. The disk asset selects reflected default-constructible systems and world services. The graphics service creates the scene, and the camera creates its View during initialization. Component changes must use declared write queries or replacement commands so change tracking can trigger extraction. See `Samples/GameSample/README.md` for build/run instructions.

`GameFramework::Application` owns common window/device/renderer/streamer initialization, world loading, the default transform/extraction schedule, rendering, and teardown. Renderer startup initializes its database and material allocator before any scene is created. Its frame loop renders after the world update completes, outside entity scheduling. Configure its `ApplicationSettings` and call `Initialize`, `Run`, then `Shutdown` on the main job fiber; stop the job system afterward. Override `ScheduleWorldUpdate` when the application needs additional phases or stages. `Run(maxFrames)` supports bounded smoke checks and leaves jobs running for cleanup.

The application constructor calls generated module anchors such as `FE::CallLinkerAnchor_GameFramework()`. Each empty function lives in its module's `Reflection.gen.cpp`, retaining that translation unit and its static registrars in static-library builds. The generator declares anchors in public `Reflection.gen.h` headers. Anchors retain reflection at link time; they do not repeat registration at runtime. AssetBuilder anchors its content modules at the asset-file loading entry point.
