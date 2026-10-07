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

`EntityCommandList::SetParent` defaults to `ReparentMode::kPreserveWorld`. TransformationSystem installs the generic Framework reparent handler during Init and clears it during Shutdown. The handler evaluates authored ancestor chains against the transactional command view, including preceding replacements and reparents, so it does not depend on cached output freshness. It computes an affine inverse, bakes optional scale into the new local matrix, and resets the modifier to identity. Invalid/non-invertible parents reject the entire command list before changing live components or links. Use explicit `kPreserveLocal` when constructing an authored hierarchy whose local values already describe its parent relationship.

Collection materialization automatically creates transient WorldTransformComponent output for authored TransformComponent through a generic runtime-companion registration. Placement roots store their authored transform in the placement asset's generic root component envelope; Framework contains no engine transform types. Cook it with `placement.m_root.CookComponent(placement.m_root.m_entities.front(), TransformComponent{ placementMatrix })` after `UpdateBindings(collection)`.
