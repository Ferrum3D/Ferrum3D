# Entity framework runtime (Stages 1-6 and Stage 7 change tracking)

`FeFramework` depends on Core only. The runtime and `FeFrameworkTests` require no graphics device. The base entity runtime, scheduler, generic change tracking and their tests stay in Framework. Engine systems live in `FeGameFramework`; its TransformationSystem begins Stage 7. Graphics systems will be implemented there in Stage 10.

## Ownership and safe points

The application owns an `EntityWorld` and its `WorldSystem` objects. Systems must remain alive until removed or the world is destroyed. `AddSystem` initializes in application order; world destruction shuts down in reverse order. Registries and entities are owned by the world. Registries group lifetime and residency, while every active registry participates in the same queries.

Initialize the engine environment/job system before constructing a world. Create registries, commit structural changes, collect systems and execute schedules on the main thread. Every producer must have completed before a structural safe point. Individual command lists have one recorder; `Submit` safely transfers completed lists into the world. Component registration is synchronized, and metadata entries remain stable for the world lifetime.

Entity addresses, IDs and UUIDs are immutable until destruction. IDs pack a process-unique 16-bit world incarnation, 24-bit slot and 24-bit generation. Destroyed IDs never resolve to later slot occupants. UUID lookup can explicitly include allocated inactive entities. Component references may be invalidated by every structural commit; cache IDs, not component pointers.

## Components and lifecycle

Reflect component types with `FE_RTTI_Reflect`, and use `FE_RTTI_Serialize` for authored fields. Framework registration uses RTTI identity, size, alignment, default/move construction, destruction and serialization. Registration asserts that component types support move construction and destruction; lifecycle hook signatures are checked at compile time. Register initialization dependencies and transient policy before creating the first instance:

```cpp
Rtti::TypeID dependencies[] = { Rtti::GetTypeID<ResourceComponent>() };
world.Components().Register<ConsumerComponent>(dependencies, { .m_transient = true });
```

Optional hook pairs are `Load/Unload`, `Init/Shutdown`, and `Activate/Deactivate`. Load takes `ComponentLoadingContext&` and returns `LifecycleResult` (succeeded, pending or failed). Its paired Unload returns void. Init/Activate take `ComponentContext&` and return a result; pending is rejected at these synchronous stages. Shutdown/Deactivate return void. A failure invokes the paired undo hook to roll back partially performed work. Load can be polled repeatedly, so it must retain its own in-progress operation state. Unload must synchronously cancel outstanding uses before returning.

All objects are constructed before hooks run. Initial initialization is children first, and activation is parents first. A whole initial subtree becomes queryable only after activation succeeds. Teardown visits descendants before parents, and reverses component initialization order. Retained components preserve lifecycle flags during storage migration. Active entities retain their existing query membership while new components or children load. A failed addition unwinds that addition; a failed replacement retains the old active value.

Runtime-created values enumerate hard references through a dependency-only sink. Cooked values use their builder-verified dependency envelopes, without serializing the payload again at runtime. Acquisitions own transitive closures through `IO::AssetRequest`; soft and optional links do not acquire residency. Ad hoc creations use lazy entity-owned residency. Use `ResidencyScope::kRegistry` on creation to share acquisitions within an ownership group. Every component contribution retains its expected-type constraint and is released after teardown. Call `AssetManager::Tick` separately to advance real asset loading; Framework does not tick the global manager.

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

Tokens belong to one list. Existing targets use `EntityID`. Lists own aligned payloads in engine arenas and support move-only values. Each entity's component changes are coalesced into one final archetype migration. Rename, parenting and value replacement use the final recorded value within a list. Destroy discards later edits to the target and its attached descendants; detach children before destruction to retain them. Independent lists that edit the same target are rejected, rather than ordered by worker completion. Invalid content identities, stale targets, missing initialization dependencies and hierarchy cycles reject the batch before publication. Foreign recorder tokens and world ownership violations assert.

After startup, new entities and components submitted in epoch N become eligible in N+1. `BeginUpdate` commits eligible commands and polls lifecycle work before collecting systems. Calling Commit repeatedly in N does not bypass publication eligibility. Existing-target edits independent of creation work can commit at the next safe point; operations that depend on list-local tokens stay with their creation batch. Lists containing hierarchy edits remain intact whenever publication is deferred, preserving detach/destroy recording order. Pending replacements retain the old value and residency until the candidate is ready. Explicit deactivation retains initialized state and residency for unchanged components. Pending replacement loading is canceled, but its committed authored value replaces the inactive old value and loads afresh on reactivation. Deliberately inactive children do not block parent readiness. Unload shuts down components and releases residency while retaining allocated identity. Registry unload cancels queued creation and pending transitions before destroying its entities and storage.

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
world.ExecuteSchedule();
world.EndUpdate(); // Releases callback captures/prerequisites; incomplete execution asserts.
// Invalid scheduling is a programmer error and asserts before callbacks run.
```

Declare application phase IDs with `FE_DECLARE_ENTITY_PHASE` at namespace scope. Phase names do not imply ordering: the order of `SchedulePhase`/`ScheduleStage` calls is authoritative. ValidateSchedule checks the complete plan before any callback runs. It asserts on repeated/omitted phases, backward phase prerequisites, null groups and dependency cycles. A stage returns its completion group and can be an explicit prerequisite. `AddPrerequisite` allows forward traversal dependencies before validation.

`Query<const T>` reads a required active component; `Query<T>` writes it. Pointer terms are optional and may be null. Callback signatures may have a leading `Entity&`. Development checks prevent component lookup through Entity from exceeding the traversal's access declarations. Exact types match; inheritance does not. Column mappings are cached by canonical archetype for each traversal; chunk rows are resolved afresh after intervening structural stages.

Completions are unsignaled when submitted and signal after execution/bookkeeping, including empty queries. Serial execution obeys prerequisites regardless of collection order. System collection cannot inspect component storage or wait on its deferred groups. Captured references must outlive the epoch; move/copy captures are arena-owned until EndUpdate. Unexecuted work is canceled and its completion released at EndUpdate, which reports failure. Callbacks run on Core job fibers. `ExecutionPolicy::kSequential` serializes a traversal's callbacks; traversals from the same system also receive exclusion edges when either uses sequential policy. `ExecutionPolicy::kParallel` uses independent chunk jobs for ordinary queries and batches up to 16 root trees per job for cascade queries. Each cascade tree completes parent callbacks before visiting children, including through ancestors excluded by self terms. Ordinary parallel queries reject parent reads that alias self writes; use a cascade query when that ordering is required.

`Without<T>` excludes entities whose archetype contains T, including components still loading or inactive. It contributes no callback argument and no component data-access conflict. For example, `Query<const Position, Without<Hidden>>` calls a callback taking only `const Position&` (or `Entity&, const Position&`). Filters can appear anywhere in the term list and can be combined with optional and parent terms. Adding or removing the excluded component invalidates changed-query snapshots through the structure revision.

`Parent<const T>` reads the required active immediate parent component; `Parent<const T*>` supplies null for a root, inactive parent, or missing component. It never searches more distant ancestors. Parent terms participate in type-wide conflicts. Entity lookup inside a callback is checked against both the declared component access and its self/parent source. Callback access context stays with the fiber across suspension and worker migration.

The scheduler validates explicit prerequisites first, then adds acyclic component write/read and write/write exclusions. Read/read traversals from independent systems or application submissions can overlap. Captured objects and external services remain the caller's synchronization responsibility. Core's non-recursive `Threading::FiberMutex` suspends a contending fiber and hands ownership to queued waiters without blocking a worker. Stage 6 also corrects the scheduler queue's empty PushFront tail handling, preventing subsequent enqueues from discarding ready jobs or fibers. Systems must not wait on work whose execution depends on their own completion.

`GetScheduleDiagnostics()` reports component conflict, system and stage edges, internal jobs, included chunks, tree batches and callbacks. `GetScheduleConflicts()` reports conflicting component IDs and the oriented traversal indices; its view lasts until the next BeginUpdate. Execution is also instrumented with profiler zones.

## Change tracking and reparent integration

```cpp
ChangeCursor renderChanges; // Retain independently for each consumer.
Query<const Position>::TraverseChanged(context, Phases::PostUpdate, renderChanges,
    [](const Position& position) { /* process changed chunks */ });
```

A cursor is caller-owned and must outlive the epoch. Use a separate cursor for each consumer/query; consuming one cursor twice in an epoch is rejected. Submission does not advance it. Completion publishes writable chunk-column versions and advances the successful consumer. Changed queries operate at chunk granularity and may conservatively invoke callbacks for untouched rows. Migration forces conservative reprocessing so pending changes cannot disappear; activation and reparenting mark affected storage changed. Cascade filtering also observes hierarchy revisions and parent-column writes, and conservatively includes all matching descendants when input changes. Direct safe-point component edits must go through replacement commands to publish changes.

`SetParent` defaults to PreserveWorld. A registered `ReparentHandler` operates on a transactional view of authored component values and hierarchy after preceding commands in the list. Its writable values are private clones until full validation succeeds. Framework owns command validation and storage publication; GameFramework owns the affine calculation. Without a transform handler/participating components, parenting changes topology. PreserveLocal bypasses the transform handler. Change-version rollover advances a separate era and invalidates older consumer cursors; exhausted era/hierarchy/structure counters fail explicitly.

## Collection assets and persistent placements

`EntityCollection` contains source UUIDs, names, internal parent UUIDs, component envelopes and one compact cooked byte buffer. `CookComponent` uses the actual serializer to capture type/version/schema and nested asset dependency metadata. `Validate` checks envelopes, bounds, duplicate identities/types, internal parents and cycles. `ValidatePayloads` additionally decodes authored values and verifies dependency metadata for authoring/build tools. Runtime requires every authored component to be registered in the world and rejects transient components and unsupported schemas.

`EntityCollectionInstanceAsset` holds a collection link, persistent root UUID, generic root component envelopes and exact source-to-concrete UUID bindings. Set the collection link before calling `UpdateBindings`: unchanged sources retain concrete UUIDs, new sources receive fresh UUIDs and removed sources lose bindings. `MakeIndependentCopy` creates a fresh root and bindings and remaps references inside cooked root components; it returns false without modifying the original on malformed data. Collection source references are remapped when materialized. Engine placement transforms belong to GameFramework and use the generic root envelope.

```cpp
EntityCollection collection;
EntityRecord source;
source.m_uuid = Uuid::Random();
source.m_name = "example";
collection.CookComponent(source, Position{ /* authored values */ });
collection.m_entities.push_back(std::move(source));

const MaterializationToken spawned = world.SpawnCollection(registry, collection);
// Or: world.SpawnCollection(registry, collectionAssetId);
// Or: world.LoadPlacement(registry, placementAssetId);
world.BeginUpdate();
world.EndUpdate();
const MaterializationStatus status = world.GetMaterializationStatus(spawned);
```

Requests are admitted on the main thread; asset discovery/loading runs asynchronously. Both in-memory definitions and real AssetManager-backed definitions use the same safe-point materialization path. Startup `CommitBootstrap` can materialize immediately; after startup publication waits until the next epoch. Tokens include the world incarnation. Status is Pending, Ready, Failed or Canceled, with a root runtime ID and diagnostic. `GetMaterializationBindings` exposes the operation's concrete UUID table. Runtime spawns receive fresh identities and a group root; authored placements restore their root and bindings. All collection roots attach beneath their group root using authored local composition.

Duplicate requests for the same placement asset return its existing pending/ready token even before publication. Conflicting placement assets claiming an occupied root/concrete UUID fail. The whole hierarchy and schemas are validated before allocating rows. Payloads deserialize into default-constructed final chunk storage; malformed payloads roll back all allocated rows. Publication waits for the complete group lifecycle/dependency closure. Independent runtime acquisitions exist before definition requests are released. Generation reads are short-lived and never cached in runtime entities; consumed definition buffers are released too. AssetManager must be ticked by the application. Entity and placement assets use AssetManager's default streamer; no streamer registration is required.

`CancelMaterialization` releases definition requests and destroys any created membership, including a ready group. Registry removal cancels pending and ready memberships before releasing the owner. A failed/canceled placement can be requested again; reload preserves authored UUIDs but allocates new generation-checked runtime IDs. Deleting individual entities from a ready placement does not respawn them. Destroying its membership root retires that membership at the next safe point.

`EntityReference` stores a concrete entity UUID and optional placement asset ID. It never acquires asset residency. `Resolve(world)` returns an active target or null; `Resolve(world, false)` permits allocated inactive targets. Resolution performs a fresh world lookup, so stream-in can resolve the same UUID to a new runtime identity. Internal source references remap during deserialization; external UUIDs remain unchanged. Remapped references clear the optional old placement diagnostic because ownership changed.

AssetBuilder provides `ImportEntityCollection(output, collection)` and `ImportEntityPlacement(output, placement, collection)` in `AssetBuilder/EntityImport.h`. They validate cooked data, preserve product identity on reimport, and save authored `.asset` definitions. The existing `BuildAsset` pipeline writes their artifacts and replays opaque component dependencies. Soft/optional targets are recorded but do not require built metadata or participate in the build dependency closure; hard dependencies retain normal type/build validation. The CLI's serialized-type import path also registers collection, placement and GameFramework transform types.

Tests remain under Framework: `FeFrameworkTests` covers the generic runtime and deterministic materialization; `FeFrameworkAssetTests` exercises real import/build/load, transitive residency, cancellation and persistent reload. Engine transformation tests are in `FeGameFrameworkTests`.

## Runtime storage and validation

Entity metadata uses a 96-byte layout on the current 64-bit target. Hierarchy links use `Memory::ShortPtr`; the world is obtained from the registry. Readiness is stored directly in Entity. Asset contributions, per-entity residency, and pending replacements share an 80-byte resource block allocated only when needed. Empty entities and ordinary data components allocate no separate runtime state. World and command-list implementations remain private: the former hides scheduler/asset storage, and the latter transfers ownership of its arena and payloads on submission. Their declarations live in separate internal headers; lifecycle resource metadata has its own header.

Command records occupy 128 bytes, component envelopes 64 bytes, and authored entity records 160 bytes. Transactional command processing prepares component lists only for component edits or transform inspection. Metadata-only edits avoid copying every entity's component list. Validation and migration share cached canonical archetypes; failed dependency layouts are rejected before entering the cache. Materialization keeps record and payload ownership together in a single temporary array.

Runtime content checks reject invalid envelopes, payload bounds, schemas, hierarchy, or bindings with `Invalid entity asset`. Asset unavailability, UUID ownership conflicts, and lifecycle failure remain distinct recoverable outcomes. Internal companion registration and API sequencing are engine invariants and use assertions. Cooked content continues to fail without assertions. Generated serialization uses reflected field names; no legacy naming aliases are retained. Rebuild entity assets after the schema changes.

Scheduling plans are programming contracts: omitted/duplicate phases, invalid prerequisites, dependency cycles, unsafe query policies, cursor reuse, and incomplete execution assert before publication. Content validation and failed asynchronous loading remain recoverable. Public header comments document borrowing and sequencing. `EntityScheduler` owns callback storage, systems, dependency validation, and dispatch as an embedded world member. Entity/chunk storage, pending commands, and loading context are separate state groups; all other world implementation methods live in `EntityWorld.cpp`.

Coarse profiler zones measure UUID random-source calls, transaction commit, materialization, lifecycle advancement, collection validation/cooking, and schedule collection/validation/execution. Entity-row callbacks, getters, and recursive lifecycle helpers do not add individual zones. UUIDs come from `Uuid::Random()`; version/variant bits are set once in Core, and random-source failure returns a null UUID.
