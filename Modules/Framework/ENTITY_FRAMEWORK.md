# Entity framework runtime (Stages 1-10)

`FeFramework` depends on Core only. The runtime and `FeFrameworkTests` require no graphics device. The base entity runtime, scheduler, generic change tracking and their tests stay in Framework. Engine systems live in `FeGameFramework`; it owns transformation, streaming and graphics systems. GameSample loads a cooked disk world through the same asset/runtime path.

## Ownership and safe points

The application owns an `EntityWorld` and its `WorldSystem` objects. Systems must remain alive until removed or the world is destroyed. `AddSystem` initializes in application order; world destruction shuts down in reverse order. Registries and entities are owned by the world. Registries group lifetime and residency, while every active registry participates in the same queries.

Registry runtime IDs are 32-bit values allocated by a thread-safe world-local bit vector that grows as needed. Zero is reserved. Recorded creation and registry-unload commands retain an ID until consumed or discarded, so removing a registry cannot make a stale command refer to a replacement registry. Registry removal cancels its active materializations before releasing its ID. Command lists must be consumed or destroyed before their world.

Initialize the engine environment/job system before constructing a world. Create registries, commit structural changes, collect systems and execute schedules on the main thread. Every producer must have completed before a structural safe point. Individual command lists have one recorder; `Submit` safely transfers completed lists into the world. Component registration is synchronized, and metadata entries remain stable for the world lifetime.

Entity addresses, IDs and UUIDs are immutable until destruction. IDs pack a process-unique 16-bit world incarnation, 24-bit slot and 24-bit generation. Destroyed IDs never resolve to later slot occupants. UUID lookup can explicitly include allocated inactive entities. Component references may be invalidated by every structural commit; cache IDs, not component pointers.

## Components and lifecycle

Reflect component types with `FE_RTTI_Reflect`, and use `FE_RTTI_Serialize` for authored fields. Framework registration uses RTTI identity, size, alignment, default/move construction, destruction and serialization. Registration asserts that component types support move construction and destruction; lifecycle hook signatures are checked at compile time. Register transient policy before creating the first instance:

```cpp
world.Components().Register<ConsumerComponent>({ .m_transient = true });
```

Optional hook pairs are `Load/Unload`, `Init/Shutdown`, and `Activate/Deactivate`. Load takes `ComponentLoadingContext&` and returns `LifecycleResult` (succeeded, pending or failed). Its paired Unload returns void. Init/Activate take `ComponentContext&` and return a result; pending is rejected at these synchronous stages. Shutdown/Deactivate return void. A failure invokes the paired undo hook to roll back partially performed work. Load can be polled repeatedly, so it must retain its own in-progress operation state. Unload must synchronously cancel outstanding uses before returning.

All objects are constructed before hooks run. Component hook order is unspecified: hooks must not depend on another component's initialized or activated runtime state. Use scheduled queries with addition filters and explicit traversal prerequisites for initialization involving several components. Initial initialization is children first, and activation is parents first. A whole initial subtree becomes queryable only after activation succeeds. Teardown visits descendants before parents. Retained components preserve lifecycle flags during storage migration. Active entities retain their existing query membership while new components or children load. A failed addition unwinds that addition; a failed replacement retains the old active value.

Runtime-created values enumerate authored hard references through Core's dependency visitor. Materialization collects the same references while copying serialized fields into runtime storage, then installs component-owned contributions after the copy succeeds. Acquisitions own transitive closures through `IO::AssetRequest`; soft and optional links do not acquire residency. Ad hoc creations use lazy entity-owned residency. Use `ResidencyScope::kRegistry` on creation to share acquisitions within an ownership group. Every component contribution retains its expected-type constraint and is released after teardown. Residency shares requests by asset ID and expected type; incompatible typed consumers cannot fail a valid consumer's acquisition. Call `AssetManager::Tick` separately to advance real asset loading; Framework does not tick the global manager.

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

Creation tokens occupy four bytes: a globally reserved 16-bit list ID and a 16-bit creation index. ID zero and index 65,535 are invalid. A synchronized fixed bitset pool supports 65,535 simultaneously live list identities; a second bitset retains IDs shared by the immediate and deferred submission fragments until both are destroyed. Each list can create at most 65,535 entities. Tokens belong to one list and must not be used after it is consumed or destroyed. Existing targets use `EntityID`. Lists own aligned payloads in engine arenas and support move-only values. Each entity's component changes are coalesced into one final archetype migration. Rename, parenting and value replacement use the final recorded value within a list. Destroy discards later edits to the target and its attached descendants; detach children before destruction to retain them. Independent lists that edit the same target are rejected, rather than ordered by worker completion. Invalid content identities, stale targets and hierarchy cycles reject the batch before publication. Foreign recorder tokens and world ownership violations assert.

After startup, new entities and components submitted in epoch N become eligible in N+1. `BeginUpdate` commits eligible commands and polls lifecycle work before collecting systems. Calling Commit repeatedly in N does not bypass publication eligibility. Existing-target edits independent of creation work can commit at the next safe point; operations that depend on list-local tokens stay with their creation batch. Lists containing hierarchy edits remain intact whenever publication is deferred, preserving detach/destroy recording order. Pending replacements retain the old value and residency until the candidate is ready. Explicit deactivation retains initialized state and residency for unchanged components. Pending replacement loading is canceled, but its committed authored value replaces the inactive old value and loads afresh on reactivation. Deliberately inactive children do not block parent readiness. Unload shuts down components and releases residency while retaining allocated identity. Registry unload cancels queued creation and pending transitions before destroying its entities and storage.

## Deferred queries and application scheduling

```cpp
void MovementSystem::Update(EntityUpdateContext& context)
{
    auto reset = Query<Velocity>::Create().Traverse(context, Phases::PreUpdate,
        [](Velocity& velocity) { /* reset */ });

    Query<const Velocity, Position, const Acceleration*>::Create().SetPrerequisites({ reset }).Traverse(
        context, Phases::PostUpdate,
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

`Query<const T>` reads a required active component; `Query<T>` writes it. Pointer terms are optional and may be null. Callback signatures may have a leading `Entity&`. Development checks prevent component lookup through Entity from exceeding the traversal's access declarations. Exact types match; inheritance does not. Column mappings are prepared per matching chunk before row iteration, and rebuilt after intervening structural stages.

Completions are unsignaled when submitted and signal after execution/bookkeeping, including empty queries. Serial execution obeys prerequisites regardless of collection order. Internal dependencies are resolved and deduplicated during graph validation; external wait groups remain separate. Each phase stores its compiled traversal order. Per-epoch scheduling data (query accesses, dependency lists, phase orders and traversal job nodes) lives in the scheduler's epoch arena; matched chunks, row ranges and batch job nodes live in the executing fiber's temporary allocator, so steady-state execution performs no heap allocation beyond pooled wait groups. Callback counts are accumulated once per batch. System collection cannot inspect component storage or wait on its deferred groups. Captured references must outlive the epoch; move/copy captures are arena-owned until EndUpdate. Unexecuted work is canceled and its completion released at EndUpdate, which reports failure. Callbacks run on Core job fibers. `ExecutionPolicy::kSequential` serializes a traversal's callbacks. `ExecutionPolicy::kMainThread` has the same sequential behavior and runs the traversal on a main-thread fiber, without dispatching chunk or tree jobs. Traversals from the same system also receive exclusion edges when either uses a sequential policy; main-thread affinity alone does not exclude worker traversals from other systems. `ExecutionPolicy::kParallel` uses independent chunk jobs for ordinary queries and batches up to 16 root trees per job for cascade queries. Each cascade tree completes parent callbacks before visiting children, including through ancestors excluded by self terms. Ordinary parallel queries reject parent reads that alias self writes; use a cascade query when that ordering is required.

`Without<T>` excludes entities whose archetype contains T, including components still loading or inactive. It contributes no callback argument and no component data-access conflict. For example, `Query<const Position, Without<Hidden>>` calls a callback taking only `const Position&` (or `Entity&, const Position&`). Filters can appear anywhere in the term list and can be combined with optional and parent terms. Adding or removing the excluded component invalidates changed-query snapshots through the structure revision.

`Parent<const T>` reads the required active immediate parent component; `Parent<const T*>` supplies null for a root, inactive parent, or missing component. It never searches more distant ancestors. Parent terms participate in type-wide conflicts. Entity lookup inside a callback is checked against both the declared component access and its self/parent source. Callback access context stays with the fiber across suspension and worker migration.

The scheduler validates explicit prerequisites first, then adds acyclic component write/read and write/write exclusions. Read/read traversals from independent systems or application submissions can overlap. Captured objects and external services remain the caller's synchronization responsibility. Core's non-recursive `Threading::FiberMutex` suspends a contending fiber and hands ownership to queued waiters without blocking a worker. Stage 6 also corrects the scheduler queue's empty PushFront tail handling, preventing subsequent enqueues from discarding ready jobs or fibers. Systems must not wait on work whose execution depends on their own completion.

`GetScheduleDiagnostics()` reports component conflict, system and stage edges, internal jobs, included chunks, tree batches and callbacks. `GetScheduleConflicts()` reports conflicting component IDs and the oriented traversal indices; its view lasts until the next BeginUpdate. Execution is also instrumented with profiler zones.

## Change tracking and reparent integration

```cpp
ChangeCursor renderChanges; // Retain independently for each consumer.
Query<const Position>::Create().SetChangeCursor(renderChanges).Traverse(context, Phases::PostUpdate,
    [](const Position& position) { /* process changed chunks */ });
```

A cursor is caller-owned and must outlive the epoch. Use a separate cursor for each consumer/query; consuming one cursor twice in an epoch is rejected. Submission does not advance it. Completion publishes writable chunk-column versions and advances the successful consumer. Changed queries operate at chunk granularity and may conservatively invoke callbacks for untouched rows. Migration forces conservative reprocessing so pending changes cannot disappear; activation and reparenting mark affected storage changed. Cascade filtering also observes hierarchy revisions and parent-column writes, and conservatively includes all matching descendants when input changes. Direct safe-point component edits must go through replacement commands to publish changes.

Addition filters use the same persistent cursor with exact per-row publication history:

```cpp
ChangeCursor newBodies; // A persistent member of the consuming system.
Query<const BodyDefinition, BodyRuntime>::Create()
    .SetChangeCursor(newBodies)
    .SetFilter(QueryFilter::kAddedComponents)
    .SetExecutionPolicy(ExecutionPolicy::kParallel)
    .Traverse(context, Phases::PreUpdate,
        [](const BodyDefinition& definition, BodyRuntime& runtime) { /* initialize runtime */ });
```

`kNewEntities` includes entities first published since the cursor was consumed, including entities without components. `kAddedComponents` includes entities with at least one newly published queried local component; present optional terms count, while `Parent` and `Without` terms only constrain matching. An initially empty cursor includes all existing matching publications. Each filter requires a cursor and consumes it after execution, even when no rows match. Keep its query and filter stable across updates. Pending loads and failed activation are excluded until successful publication; migration, compaction, value replacement and reactivation do not create additions. Removing and subsequently adding a component does. Independent cursors let multiple systems process the same additions. Both filters support cascade ordering, prerequisites and all execution policies.

`SetParent` defaults to PreserveWorld. A registered `ReparentHandler` operates on a transactional view of authored component values and hierarchy after preceding commands in the list. Its writable values are private clones until full validation succeeds. Framework owns command validation and storage publication; GameFramework owns the affine calculation. Without a transform handler/participating components, parenting changes topology. PreserveLocal bypasses the transform handler. Change-version rollover advances a separate era and invalidates older consumer cursors; exhausted era/hierarchy/structure counters fail explicitly.

## Collection assets and persistent placements

`EntityCollection` contains source UUIDs, names, internal parent UUIDs and reflected component values (`Rtti::Any`). Generated serialization stores their authored fields and discovers nested asset links through the existing asset dependency visitor. `Validate` checks identities, component types, duplicates, internal parents and cycles. Runtime requires every authored component to be registered in the world and rejects transient components. Materialization copies serialized fields and remaps internal entity references; runtime-only fields start from their defaults.

`EntityCollectionInstanceAsset` holds a collection link, persistent root UUID, generic root component envelopes and exact source-to-concrete UUID bindings. Set the collection link before calling `UpdateBindings`: unchanged sources retain concrete UUIDs, new sources receive fresh UUIDs and removed sources lose bindings. `MakeIndependentCopy` creates a fresh root and bindings and remaps references inside cooked root components; it returns false without modifying the original on malformed data. Collection source references are remapped when materialized. Engine placement transforms belong to GameFramework and use reflected root component values.

```cpp
EntityCollection collection;
EntityRecord source;
source.m_uuid = Uuid::Random();
source.m_name = "example";
source.AddComponent(Position{ /* authored values */ });
collection.m_entities.push_back(std::move(source));

const MaterializationToken spawned = world.SpawnCollection(registry, collection);
// Or: world.SpawnCollection(registry, collectionAssetId);
// Or: world.LoadPlacement(registry, placementAssetId);
world.BeginUpdate();
world.EndUpdate();
const MaterializationStatus status = world.GetMaterializationStatus(spawned);
```

Requests are admitted on the main thread; asset discovery/loading runs asynchronously. Both in-memory definitions and real AssetManager-backed definitions use the same safe-point materialization path. Startup `CommitBootstrap` can materialize immediately; after startup publication waits until the next epoch. Tokens include the world incarnation. Status is Pending, Ready, Failed or Canceled, with a root runtime ID and diagnostic. `GetMaterializationBindings` exposes the operation's concrete UUID table. Runtime spawns receive fresh identities and a group root; authored placements restore their root and bindings. All collection roots attach beneath their group root using authored local composition.

Duplicate requests for the same placement asset return its existing pending/ready token even before publication. Conflicting placement assets claiming an occupied root/concrete UUID fail. The whole hierarchy and registered component types are validated before allocating rows. Serialized fields are copied into default-constructed final chunk storage; failed copies roll back all allocated rows. Publication waits for the complete group lifecycle/dependency closure. Independent runtime acquisitions exist before definition requests are released. A placement request already retains its hard collection dependency, so no second definition acquisition is needed. Asset-backed definitions are read in place: the operation pins the published placement and collection generations while rows are constructed and releases the pins with the definition request; runtime entities never cache reads. The asset manager runs each type's `Validate()` once when it loads an asset, so cooked placements and collections are not revalidated; materialization only checks that placement bindings match the collection, plus world state (UUID ownership and registered component types). In-memory definitions are validated once, immediately before rows are allocated. Core request diagnostics are copied before failed requests are released. Pending requests and live memberships are tracked separately from completed token history, and placement assets have a lookup index. AssetManager must be ticked by the application. Entity and placement assets use AssetManager's default streamer; no streamer registration is required.

`CancelMaterialization` releases definition requests and destroys any created membership, including a ready group. Registry removal cancels pending and ready memberships before releasing the owner. A failed/canceled placement can be requested again; reload preserves authored UUIDs but allocates new generation-checked runtime IDs. Deleting individual entities from a ready placement does not respawn them. Destroying its membership root retires that membership at the next safe point.

`EntityReference` stores a concrete entity UUID and optional placement asset ID. It never acquires asset residency. `Resolve(world)` returns an active target or null; `Resolve(world, false)` permits allocated inactive targets. Resolution performs a fresh world lookup, so stream-in can resolve the same UUID to a new runtime identity. Internal source references remap during deserialization; external UUIDs remain unchanged. Remapped references clear the optional old placement diagnostic because ownership changed.

Collections, placements, worlds and snapshots are normal source-less serialized assets. Create or refresh their `.asset` files with `ferrum import --asset-type <TYPE_UUID> --output <PATH>` and cook them with `ferrum build --asset <PATH> --output <CATALOG>`. The existing AssetBuilder pipeline preserves identity on reimport, discovers nested component links and writes the artifacts. Soft/optional targets do not participate in the build dependency closure; hard dependencies retain normal type/build validation. There is no entity-specific importer, dependency envelope or compiler.

Tests remain under Framework: `FeFrameworkTests` covers the generic runtime, deterministic materialization, real import/build/load, transitive residency, cancellation and persistent reload. Engine transformation tests are in `FeGameFrameworkTests`.

## World definitions, snapshots and streaming

`EntityWorldAsset` selects systems and services by reflected type IDs and describes registries using persistent UUID keys, concrete collections and soft placement links. `EntityWorldInstance::Load` default-constructs the selected `WorldService` types, then the `WorldSystem` types, through reflection, then loads the definition into its independent world. Systems and services use engine allocation and type-erased ownership. Unknown, duplicate, abstract, incompatible and non-default-constructible types reject loading; reflection casts adjust base addresses for multiple inheritance. Services survive entity teardown and system shutdown. `EntityWorld::AddService` and `RemoveService` support explicit borrowed registration, and `FindService<T>` resolves reflected service types or bases. World services update before structural commit without recording scheduled traversals. Component hooks access them through the owning world. `GetStatus` reports completion or failure of the initial materialization. Tick AssetManager while pending; use `CommitBootstrap` only during startup. A registry's key survives save/reload while its runtime ID is newly allocated.

`EntityWorldInstance::Capture` saves the selected system type IDs plus current concrete entities, names, sibling order, hierarchy, authored components, activation intent, residency scope and surviving placement membership. Computed/transient components are omitted. Capture runs at a closed main-thread boundary and rejects pending commands or lifecycle/materialization transitions; `GetLastError` reports the blocking category. `Restore` reconstructs this concrete state directly, recreates transient companions and reacquires dependencies; it does not reload the original collection and therefore does not recreate deleted source entities. Placement bookkeeping retains original bindings for reference diagnostics without introducing a hard dependency on the original placement asset. UUIDs persist and runtime IDs change.

GameFramework's `WorldStreamingService` accepts main-thread placement load and registry unload requests. `WorldService::Update` applies requests before the world's safe-point commit and traversal collection. Unloading cancels pending materialization before destroying membership; a later load preserves concrete UUIDs with fresh runtime handles. Every loaded registry participates in the same world queries and schedule.

`RecordStage` records a main-thread stage in a phase that the application will schedule explicitly. Use it for application work without query terms; `ScheduleStage` additionally inserts that phase into the application schedule. Systems that update external scene/view state from components can use `ExecutionPolicy::kMainThread` to apply changes directly during a traversal.

`FeFrameworkTests` covers independent world instances and concrete snapshot restoration, and cooks, loads and round-trips world/snapshot assets through the default streamer. `FeGameFrameworkTests` covers asynchronous registry streaming, cancellation, failed loads and graphics extraction. See `Samples/GameSample/README.md` for the two-entity disk-world example.

## Runtime storage and validation

Entity metadata uses a 96-byte layout on the current 64-bit target. Hierarchy links use `Memory::ShortPtr`; the world is obtained from the registry. Readiness is stored directly in Entity. Safe points advance pending replacements and dirty hierarchy roots instead of scanning every allocated slot. Failure cleanup performs one children-first deactivation pass followed by one children-first teardown pass. Asset contributions, per-entity residency, and pending replacements share an 80-byte resource block allocated only when needed. Empty entities and ordinary data components allocate no separate runtime state. World and command-list implementations remain private: the former hides scheduler/asset storage, and the latter transfers ownership of its arena and payloads on submission. Their declarations live in separate internal headers; lifecycle resource metadata has its own header. Command transactions build a sparse overlay of targets and affected hierarchy paths, then record, validate and publish in separate stages. Same-layout replacements retain the existing row and unrelated values; only layout changes migrate storage. Available chunks are indexed by archetype and registry, so allocation does not scan all chunks.

Command records occupy 64 bytes, using tagged unions for ID/token targets and kind-specific payloads; reflected authored component slots use 16-byte `Rtti::Any` values. Authored records also retain activation intent and residency scope. Transactional command processing prepares component lists only for component edits or transform inspection. Metadata-only edits avoid copying every entity's component list. Validation and migration share cached canonical archetypes; failed dependency layouts are rejected before entering the cache. Materialization reads owning reflected values directly from the temporary definition.

Runtime content checks reject unsupported component types, duplicate components, invalid hierarchy, or bindings with `Invalid entity asset`. Asset unavailability, UUID ownership conflicts, and lifecycle failure remain distinct recoverable outcomes. Internal companion registration and API sequencing are engine invariants and use assertions. Cooked content continues to fail without assertions. Generated serialization uses reflected field names; no legacy naming aliases are retained. Rebuild entity assets after the schema changes.

Scheduling plans are programming contracts: omitted/duplicate phases, invalid prerequisites, dependency cycles, unsafe query policies, cursor reuse, and incomplete execution assert before publication. Content validation and failed asynchronous loading remain recoverable. Public header comments document borrowing and sequencing. The private `EntityWorld::Impl` owns entity/chunk storage and structural edits (`EntityWorld.cpp`, `EntityTransaction.cpp`) and delegates to embedded members: `EntityScheduler` owns callback storage, systems, dependency validation and dispatch; `EntityLifecycle` owns the component load/init/activate state machine, per-entity asset contributions, pending replacements and the lifecycle quota; `EntityMaterializer` owns collection/placement operations, their tokens and the materialization quota. Definitions and snapshots live in `EntityWorldPersistence.cpp`. A commit applies command lists, advances materializations, advances lifecycle, then publishes materialization readiness.

Coarse profiler zones measure UUID random-source calls, transaction commit, materialization, lifecycle advancement, collection validation/cooking, and schedule collection/validation/execution. Entity-row callbacks, getters, and recursive lifecycle helpers do not add individual zones. UUIDs come from `Uuid::Random()`; version/variant bits are set once in Core, and random-source failure returns a null UUID.
