# Entity framework runtime (Stages 1–5)

`FeFramework` depends on Core only. The runtime and `FeFrameworkTests` require no graphics device. Parallel policies, executing Parent/Cascade terms, transforms, and asset materialization belong to the later milestones.

## Ownership and safe points

The application owns an `EntityWorld` and its `WorldSystem` objects. Systems must remain alive until removed or the world is destroyed. `AddSystem` initializes in application order; world destruction shuts down in reverse order. Registries and entities are owned by the world. Registries group lifetime and residency, while every active registry participates in the same queries.

Initialize the engine environment/job system before constructing a world. Create registries, commit structural changes, collect systems and execute schedules on the main thread. Every producer must have completed before a structural safe point. Individual command lists have one recorder; `Submit` safely transfers completed lists into the world. Component registration is synchronized, and metadata entries remain stable for the world lifetime.

Entity addresses, IDs and UUIDs are immutable until destruction. IDs pack a process-unique 16-bit world incarnation, 24-bit slot and 24-bit generation. Destroyed IDs never resolve to later slot occupants. UUID lookup can explicitly include allocated inactive entities. Component references may be invalidated by every structural commit; cache IDs, not component pointers.

## Components and lifecycle

Reflect component types with `FE_RTTI_Reflect`, and use `FE_RTTI_Serialize` for authored fields. Framework registration uses RTTI identity, size, alignment, default/move construction, destruction and serialization. Registration rejects types without no-throw relocation; `Components().GetLastError()` explains rejection. Register initialization dependencies and transient policy before creating the first instance:

```cpp
Rtti::TypeID dependencies[] = { Rtti::GetTypeID<ResourceComponent>() };
world.Components().Register<ConsumerComponent>(dependencies, { .m_transient = true });
```

Optional hook pairs are `Load/Unload`, `Init/Shutdown`, and `Activate/Deactivate`. Load takes `ComponentLoadingContext&` and returns `LifecycleResult` (succeeded, pending or failed). Its paired Unload returns void. Init/Activate take `ComponentContext&` and return a result; pending is rejected at these synchronous stages. Shutdown/Deactivate return void. A failure invokes the paired undo hook to roll back partially performed work. Load can be polled repeatedly, so it must retain its own in-progress operation state. Unload must synchronously cancel outstanding uses before returning.

All objects are constructed before hooks run. Initial initialization is children first, and activation is parents first. A whole initial subtree becomes queryable only after activation succeeds. Teardown visits descendants before parents, and reverses component initialization order. Retained components preserve lifecycle flags during storage migration. Active entities retain their existing query membership while new components or children load. A failed addition unwinds that addition; a failed replacement retains the old active value.

Actual serializers enumerate hard references through a dependency-only sink. Acquisitions own transitive closures through `IO::AssetRequest`; soft and optional links do not acquire residency. Ad hoc creations use lazy entity-owned residency. Use `ResidencyScope::kRegistry` on creation to share acquisitions within an ownership group. Every component contribution retains its expected-type constraint and is released after teardown. Call `AssetManager::Tick` separately to advance real asset loading; Framework does not tick the global manager.

## Commands and epochs

```cpp
EntityCommandList commands(world);
auto root = commands.CreateEntity(registry, "root");
auto child = commands.CreateEntity(registry, "child");
commands.AddComponent(root, Position{ /* authored values */ });
commands.AddComponent(child, Position{ /* authored values */ });
commands.SetParent(child, root);
world.Submit(std::move(commands));
world.CommitBootstrap(); // Available only before the first BeginUpdate.
```

Tokens belong to one list. Existing targets use `EntityID`. Lists own aligned payloads in engine arenas and support move-only values. Each entity's component changes are coalesced into one final archetype migration. Rename, parenting and value replacement use the final recorded value within a list. Destroy discards later edits to the target and its attached descendants; detach children before destruction to retain them. Independent lists that edit the same target are rejected, rather than ordered by worker completion. Invalid UUIDs/targets, foreign registries, missing initialization dependencies and cycles reject the batch before publication.

After startup, new entities and components submitted in epoch N become eligible in N+1. `BeginUpdate` commits eligible commands and polls lifecycle work before collecting systems. Calling Commit repeatedly in N does not bypass publication eligibility. Existing-target edits independent of creation work can commit at the next safe point; operations that depend on list-local tokens stay with their creation batch. Pending replacements retain the old value and residency until the candidate is ready. Explicit deactivation retains initialized state and residency; Unload shuts down and releases it while retaining allocated identity. Registry unload cancels queued creation and pending transitions before destroying its entities and storage.

## Deferred queries and application scheduling

```cpp
void MovementSystem::Update(EntityUpdateContext& context)
{
    auto reset = Query<Velocity>::Traverse(context, Phases::PreUpdate,
        [](Velocity& velocity) { /* reset */ });

    Query<const Velocity, Position, const Acceleration*>::Traverse(
        context, Phases::PostUpdate, { reset },
        [](Entity& entity, const Velocity& velocity, Position& position,
           const Acceleration* acceleration) { /* optional component may be null */ });
}

world.BeginUpdate(); // Collects every system once; Traverse executes no callbacks here.
world.SchedulePhase(Phases::PreUpdate);
world.ScheduleStage(ApplicationStage, [&] { /* application work or a safe structural commit */ });
world.SchedulePhase(Phases::PostUpdate);
const bool executed = world.ExecuteSchedule();
world.EndUpdate(); // Cancels unfinished records on failure and releases captures/prerequisites.
// Handle !executed using world.GetLastError().
```

Declare application phase IDs with `FE_DECLARE_ENTITY_PHASE` at namespace scope. Phase names do not imply ordering: the order of `SchedulePhase`/`ScheduleStage` calls is authoritative. ValidateSchedule checks the complete plan before any callback runs. It rejects repeated/omitted phases, backward phase prerequisites, null groups and dependency cycles. A stage returns its completion group and can be an explicit prerequisite. `AddPrerequisite` allows forward traversal dependencies before validation.

`Query<const T>` reads a required active component; `Query<T>` writes it. Pointer terms are optional and may be null. Callback signatures may have a leading `Entity&`. Development checks prevent component lookup through Entity from exceeding the traversal's access declarations. Exact types match; inheritance does not. Column mappings are cached by canonical archetype for each traversal; chunk rows are resolved afresh after intervening structural stages.

Completions are unsignaled when submitted and signal after execution/bookkeeping, including empty queries. Serial execution obeys prerequisites regardless of collection order. System collection cannot inspect component storage or wait on its deferred groups. Captured references must outlive the epoch; move/copy captures are arena-owned until EndUpdate. Unexecuted work is canceled and its completion released at EndUpdate, which reports failure. Parent read-only terms, CascadeQuery and the parallel policy names compile for API examples; execution explicitly rejects those features until Stage 6. The default and explicitly selected `ExecutionPolicy::kSequential` both execute serially.
