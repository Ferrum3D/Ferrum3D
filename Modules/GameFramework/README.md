# GameFramework

`FeGameFramework` is the separate engine-system project. It depends on `FeFramework`, which retains the reusable entity runtime and its GPU-independent tests. GameFramework owns transformation, graphics and streaming components and systems; other engine systems belong here as well. It also depends on Graphics for scene integration.

## Stage 7 implementation

- Reflected, serialized `TransformComponent` stores an authoritative `FE::Transform` (translation, quaternion rotation, and uniform scale).
- Optional authored `NonUniformScaleComponent` supplies a scale modifier.
- `WorldTransformComponent` stores transient computed output. TransformationSystem registers its transient policy before entities are created.
- `TransformationSystem` uses changed cascade queries and parallel batches within each hierarchy level. Siblings within one tree can overlap; each level joins before children start. Unchanged epochs skip transform callbacks. Parent/local/scale changes, new children and hierarchy changes conservatively propagate through matching descendants.

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

Commands and collection materialization automatically create transient WorldTransformComponent output for authored TransformComponent through a generic runtime-companion registration. Placement roots store their authored transform in the placement asset's reflected root component values; Framework contains no engine transform types. Add it with `placement.m_root.m_entities.front().AddComponent(TransformComponent{ placementTransform })` after `UpdateBindings(collection)`.

Changing local storage from a matrix to `FE::Transform` changes the generated serialization schema. Rebuild cooked assets containing TransformComponent.


CameraSystem and MeshSystem register mesh and camera components with lifecycle hooks. Their runtime fields use `FE_SKIP_SERIALIZING`, so world assets and snapshots contain only authored settings. Copy construction copies those settings without duplicating graphics ownership; component replacement initializes new membership before releasing the previous value. WorldGraphicsSceneService creates and owns each world's scene and mesh bridge. Applications order Transformation before GraphicsExtraction and render after the world update completes. Changed queries update View matrices and MeshSceneModule model/material membership on the main thread, then extract mesh transforms with `ExecutionPolicy::kParallel`. Membership stays stable until all transform jobs join. Workers write distinct preallocated GPU database rows; the scene publishes dirty page and batch bits afterward, without retaining component addresses in the renderer. Invalid camera updates leave the previous matrices usable. Each camera creates its own View during component initialization, enables it during activation, and removes it during shutdown.

WorldStreamingService queues placement loads and registry unloads by persistent registry key. Its service update applies requests before structural commit; pending unloads cancel materialization and residency. GetStatus reports pending, ready, failed, or canceled operations. Registries do not split simulation queries or scheduling.

GameSample builds and loads a cooked world with 50 dynamic helmet/bunny rigid bodies in five layers of ten, a static box floor, and one camera. The disk asset selects reflected default-constructible systems and world services. The graphics service creates the scene, and the camera creates its View during initialization. Component changes must use declared write queries or replacement commands so change tracking can trigger extraction. See `Samples/GameSample/README.md` for build/run instructions.

`GameFramework::Application` owns common window/device/renderer/streamer initialization, world loading, the default transform/extraction schedule, rendering, and teardown. Renderer startup initializes its database and material allocator before any scene is created. Its frame loop renders after the world update completes, outside entity scheduling. Configure its `ApplicationSettings` and call `Initialize`, `Run`, then `Shutdown` on the main job fiber; stop the job system afterward. Override `ScheduleWorldUpdate` for per-frame ordering and `ScheduleWorldFixedUpdate` for fixed-step ordering when the application needs additional phases or stages. `Run(maxFrames)` supports bounded smoke checks and leaves jobs running for cleanup.

The application constructor calls generated module anchors such as `FE::CallLinkerAnchor_GameFramework()`. Each empty function lives in its module's `Reflection.gen.cpp`, retaining that translation unit and its static registrars in static-library builds. The generator declares anchors in public `Reflection.gen.h` headers. Anchors retain reflection at link time; they do not repeat registration at runtime. AssetBuilder anchors its content modules at the asset-file loading entry point.

## Primitive rigid bodies

Register `WorldPhysicsService` and `PhysicsSystem` before loading entities. A `PhysicsBodyComponent` contains a list of `PhysicsBodyShape` entries, each with an inline sphere, box, capsule, or cylinder definition and an optional shared `PhysicsShapeAsset` link. The asset overrides the inline definition. Offsets and rotations are relative to the entity origin; multiple entries become a Jolt static compound shape. Box dimensions are half extents; capsule/cylinder length uses the half height of the cylindrical section along local Y. Units are meters, kilograms, and seconds.

Bodies require a TransformComponent at a transform root (organizational parents without transforms are allowed). Initial scale is baked into the collision shape; unsupported primitive scaling fails activation. Replace PhysicsBodyComponent to rebuild collision geometry or change mass/material properties. Keep active rigid bodies at transform roots and replace the body after changing collision scale. Authored transform edits teleport static and dynamic bodies at the next step; kinematic bodies move toward their authored transform every step. Dynamic transforms are published after simulation, and only bodies that moved report a change, so resting bodies do not trigger transform propagation or extraction. Initial velocities are authored fields and dynamic velocities are updated after simulation, so snapshots retain motion. Copies and serialization exclude runtime handles; replacement, deactivation, registry unloading, and teardown release body membership.

`EntityWorld::BeginFixedUpdate(deltaSeconds)` opens an independent epoch that invokes `WorldSystem::FixedUpdate` without ticking frame services. Update frequency is selected by `Update` versus `FixedUpdate`; phase keys only specify ordering and can be used at either frequency. The default fixed-step schedule is `Framework::Phases::PreUpdate` (synchronize authored body changes), `Update` (simulate physics), then `PostUpdate` (publish transforms and velocities). PhysicsSystem records all three operations from its `FixedUpdate` hook. Schedule the required phases, execute, and end the epoch like a normal update. The per-frame and fixed-step application schedules have independent ordering hooks. `Framework::EntityWorld` accumulates elapsed time and runs 1/60-second steps before its normal input/transform/extraction epoch, with at most eight steps per frame and excess elapsed time discarded. BeginFrame shares lifecycle quotas across all these epochs.

Jolt uses engine allocation callbacks. With fewer than 16 active rigid bodies, its job graph executes inline on the calling engine fiber, avoiding dispatch overhead for small scenes. At 16 or more active bodies, a pooled adapter submits Jolt jobs directly as engine JobNodes. Its reusable barrier and WaitGroups drain all queued engine jobs before returning. The body-count threshold is a scheduling heuristic, not a solver setting. Jolt process registration is shared across world services and released after the last physics world shuts down. The integration exposes no mesh collision, soft bodies, characters, constraints, or contact callbacks.

The opt-in `PhysicsFixture.DISABLED_StepBenchmark` measures 512 warmed-up physics steps with ten moving spheres and a static floor, without rendering. Run `FeGameFrameworkTests.exe --gtest_filter=PhysicsFixture.DISABLED_StepBenchmark --gtest_also_run_disabled_tests` in Release. It reports mean, median, p95, and maximum time without imposing machine-dependent timing assertions. Release currently retains the engine development heap and stack tracking; benchmark comparisons should keep those settings the same.

ECS schedule validation is disabled by default. Pass `--ecs-validation` to enable query-policy and dependency checks before callbacks, including in Release. Required phase ordering and component-access exclusions remain active without validation. `EntityWorld::RunFixedUpdates` owns the fixed-step accumulator and catch-up loop; the application supplies elapsed frame time and the phase scheduling callback. Explicit `ValidateSchedule()` calls always validate.

## Rendering and UI

Each frame `Application::BuildRenderRequest` tells the renderer which View renders into which `Core::Viewport`. The default adds the world's camera views to the main viewport, followed by one UI view per Dear ImGui viewport (`Graphics::ImGuiRenderer`). View modules publish their data in `Update`; the renderer then adds passes in a fixed order, and each pass skips itself when its module's data is missing. Scene passes render into view-owned targets before the renderer releases scene state; output passes (tonemapping, UI) write the acquired viewport image afterwards. All acquired viewports are presented by a single submission. Override `DrawImGuiWindows` (declared on `Framework::Application`) to add windows; the ImGui frame runs after `Renderer::WaitForScene`, so windows may mutate scene state. ImGui keyboard and mouse capture withholds presses from the input backend.
