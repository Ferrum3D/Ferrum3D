# GameFramework

`FeGameFramework` is the separate engine-system project. It depends on `FeFramework`, which retains the reusable entity runtime and its GPU-independent tests. GameFramework owns engine components and systems; future graphics, streaming and other engine systems belong here. Graphics integration remains Stage 10.

## Stage 7 implementation started

- Reflected, serialized `TransformComponent` stores an authoritative affine `Matrix4x4`, preserving non-uniform scale and shear.
- Optional authored `NonUniformScaleComponent` supplies a scale modifier.
- `WorldTransformComponent` stores transient computed output. TransformationSystem registers its transient policy before entities are created.
- `TransformationSystem` uses changed cascade queries and parallel tree batches. Unchanged epochs skip transform callbacks. Parent/local/scale changes, new children and hierarchy changes conservatively propagate through matching descendants.

The engine uses row vectors. Composition is `Scale(modifier) * local * parentWorld`; a missing immediate parent WorldTransform contributes identity. A node without WorldTransform does not transmit a more distant ancestor transform. Entities participate only when they have both local Transform and WorldTransform components.

```cpp
GameFramework::TransformationSystem transforms;
Framework::EntityWorld world;
world.AddSystem(transforms); // The system must outlive registration.

Framework::EntityCommandList commands(world);
auto root = commands.CreateEntity(world.CreateRegistry(), "root");
commands.AddComponent(root, GameFramework::TransformComponent{ Matrix4x4::kIdentity });
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

Still pending in Stage 7: PreserveWorld as the default SetParent behavior, explicit PreserveLocal, the generic transform reparent integration handler, modifier baking and atomic singular-parent rejection. Current SetParent preserves authored local values and changes topology only.
