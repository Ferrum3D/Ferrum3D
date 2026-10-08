# Entity framework implementation plan

Status: Stages 1–8 implemented. The reusable entity runtime, collection assets, persistent placement materialization and generic tests are in `Modules/Framework`. Affine transformation components, reparent integration and TransformationSystem are in `Modules/GameFramework`. AssetBuilder imports/cooks collection and placement definitions. Stages 9–11 remain planned. See `Modules/Framework/ENTITY_FRAMEWORK.md` and `Modules/GameFramework/README.md`.

This plan replaces the old `Modules/Framework/.../Entities` prototype with a hybrid entity/ECS framework. Entity objects provide stable identity and hierarchy. Components reside in packed archetype chunks. World systems submit deferred query work into application-scheduled phases. Assets describe worlds, reusable collections, and persistent placements; registries own runtime entity groups and residency without defining simulation boundaries.

All engine systems and engine-specific components belong to `Modules/GameFramework`; Framework retains the reusable entity runtime and its tests. Graphics resource/rendering implementations stay in Graphics, with their entity systems in GameFramework.

The decisions below are the starting contract. Changes to that contract should update this document before changing dependent APIs.

## 1. Scope and constraints

### Required behavior

- An entity has a name, immutable runtime ID, immutable serialization UUID, stable address, parent pointer, and intrusive list of children.
- Entity addresses and runtime IDs remain stable until destruction. UUIDs survive save/reload; independently copied entities receive new UUIDs.
- Components use RTTI type identity, generated serialization where applicable, and detected lifecycle functions. Components have no update function and require no polymorphic base interface.
- Component storage remains archetype-based and uses component arrays packed into chunks.
- Remove `IComponentProvider`, entity-local `EntitySystem`, bespoke persistent component IDs, and system archetype register/unregister hooks.
- A `WorldSystem` has `Init`, `Shutdown`, and `Update`. Queries are independent of systems; a system can submit multiple traversals in multiple phases.
- `Traverse` submits work and returns its completion `Rc<WaitGroup>`. It never invokes entity callbacks synchronously.
- The application explicitly schedules phases and intervening stages. Phase names/IDs do not imply order. Scheduling the same phase twice in one update epoch is an error.
- Component access modifiers determine conflicting traversals. Explicit prerequisites determine semantic ordering. Compatible work can run in parallel.
- A system defaults to sequential callback execution, including across its different traversals. Parallel execution is an explicit policy.
- Creation commands submitted in frame N cannot publish entities/components until frame N+1. Asset-free creations are ready before simulation starts in N+1.
- Registries are pure runtime ownership/residency groups. Queries see all active entities across the world equally.
- Parent/child relationships never cross registry or world boundaries.
- Reusable collection data can be released after materialization without destroying its runtime entities or releasing their required dependencies.
- Authored collection instances are assets with identities that can be referenced before they are streamed into a world.

### Initial exclusions

Property-difference prefab overrides, automatic propagation of collection edits to live instances, networking/replication, generalized relationship queries, inheritance-aware component matching, automatic asset hot-reload propagation, and concurrent structural mutation are follow-up work. The first implementation must support full snapshot saves and explicit replacement/reload without requiring those features.

The initial scheduler tracks component accesses only. Systems are responsible for synchronization of captured objects, system members, graphics scenes, and other external state. A fiber-aware mutex is supplied as a utility, not as the mechanism for resolving declared component conflicts.

## 2. Existing code and ownership boundaries

| Area | Existing foundation | Planned responsibility |
| --- | --- | --- |
| `Modules/Framework/Public/Framework/Entities` and corresponding private directory | Entity pool, registries, chunk-storage prototype, deferred actions | Replace obsolete APIs; retain useful allocator patterns and rebuild storage/lifecycle contracts |
| `FerrumCore/Public/Core/RTTI/Reflection.h` and `ReflectionContext.h` | Type identity, size/alignment, construction/destruction, serialization callbacks | Add generic move construction; keep component stages in Framework |
| `FerrumCore/Public/Core/Serialization` | Generated and type-erased serialization, schema/version metadata, asset-reference callback | Encode component payloads and enumerate serialized dependencies |
| `FerrumCore/Public/Core/IO/Assets.h` and `AssetManager.h` | Identity-only links, graph acquisitions, generation reads, hard/soft dependencies | Retain independent runtime residency; load immutable framework assets |
| `FerrumCore/Public/Core/Jobs` | Jobs, prerequisites, pooled wait groups, fiber waiting | Execute framework work graphs and completion aggregation |
| `Modules/Graphics/Framework/.../Scene` and `Features/Mesh` | Render scenes, instance handles, transform updates | Integrate through render systems in a separate layer |
| `Tools/AssetBuilder` | Serialized asset import and dependency collection | Build world, collection, placement, and snapshot artifacts |

`FeFramework` continues to depend on `FeCore`, not on `FeGraphics`. Transform components/systems belong in Framework. Graphics-specific components and render systems belong in the Graphics project, which can depend on Framework. Avoid a circular module dependency.

The existing entity prototype is not a correctness reference. Audit chunk alignment, allocation metadata initialization, allocation release, archetype-key collision handling, move-source destruction, and relocation mappings before reusing code.

## 3. Runtime identity and storage

### Entity and handle contract

- Allocate `Entity` objects from a world-owned engine pool. Each object holds its world and registry, name, UUID, runtime ID, hierarchy links, lifecycle bookkeeping, and current chunk/row location.
- Use a 64-bit runtime ID: 16-bit world incarnation token, 24-bit world-local entity slot, and 24-bit slot generation. Use explicit packing/masks rather than compiler-dependent bit-field layout.
- Reserve zero as invalid. World incarnation tokens are not recycled during a process lifetime. Reject exhaustion rather than aliasing an old world. Retire entity slots when their generation would wrap.
- The world slot table validates token, slot bounds, generation, and occupancy. A failed lookup returns null rather than resolving an unrelated object.
- Registry identity is not encoded in the runtime ID. Registry ownership is immutable in the initial implementation; transfer can be added later without changing entity identity.
- Maintain a world UUID lookup for entities that are still allocated, including pending/inactive entities. Duplicate UUID insertion fails before publication.
- Raw `Entity*` is valid only while the caller can guarantee entity lifetime. Use runtime handles for deferred commands and cached gameplay references.
- Component pointers/references are temporary. Structural commits, chunk compaction, and archetype changes can invalidate them. No persistent raw component pointers are allowed in systems or external subsystems.
- Entity metadata access during scheduled work is read-only except through command recording. Retain the optional `Entity&` callback argument, but validate component lookups against the current traversal's declarations in development builds.

### Archetypes and chunks

- World-owned archetype definitions have canonical sorted RTTI type lists and collision-checked signatures. Use dense runtime component indices internally for masks and fast lookup; never serialize those indices.
- Archetype definitions are shared between registries. Initially, chunks belong to a single registry, allowing efficient bulk destruction and residency accounting. World queries index matching chunks from every registry.
- Use fixed 16 KiB normal chunks with aligned component columns, occupancy/active masks, row-to-entity lookup, and explicit capacity/count. Oversized component layouts use an aligned larger chunk capable of holding at least one row.
- Compact occupied rows when safe; update the moved entity's location and transfer associated lifecycle/change metadata. Inactive/loading rows remain allocated but are excluded from ordinary queries.
- Components retained during migration preserve their lifecycle state. Move-construct into destination storage and destroy moved-from objects exactly once. Never invoke deactivate/activate merely because storage changed.
- Require move construction and destruction for chunk-stored components. Assert on registration of an unsupported type. Address-sensitive resources live behind stable handles or engine-managed external objects.
- Sparse transition records track pending component additions and failures; do not add a vtable or a large state structure to every component object.

## 4. RTTI and component lifecycle

### Generic RTTI extension

Add an optional move-constructor callback and a no-throw-move capability flag to `Rtti::Type`. Bind them in `ReflectionContext` using C++20 traits. Reflection registration remains the single source of generic type size, alignment, construction, move, destruction, and serialization operations.

Framework keeps a component operations table containing a reference to `Rtti::Type`, optional detected lifecycle callbacks, and component-specific policies. It does not duplicate persistent type identity or generic object operations. Plain data components need only reflected type registration.

### Component stages

The public sequence is:

```text
allocate/construct -> deserialize or assign initial values
                   -> load -> initialize -> activate
                   -> deactivate -> shutdown -> unload -> destroy/deallocate
```

- Detect `Load`, `Unload`, `Init`, `Shutdown`, `Activate`, and `Deactivate` with concepts/traits. Keep paired hooks consistent and validate signatures at registration.
- Hooks receive stage-appropriate context, including entity identity, world services, and loading/residency access. Context does not expose unrestricted concurrent structural mutation.
- Load supports pending/succeeded/failed results. Framework acquires serialized hard dependencies first; custom loading can register additional required requests through the same owner and remain pending until ready.
- Init and Activate are synchronous, non-blocking transitions. Use a result-bearing wrapper for development diagnostics/rollback; pending work belongs in Load.
- Execute lifecycle publication and teardown at main-thread safe points initially. Deserialization and dependency discovery can run in background fibers. This permits graphics and other thread-affine integrations without arbitrary lifecycle races.
- Absent hooks are successful no-ops. Generated serialization stores authored state only; runtime handles, caches, and computed output components are omitted or registered as transient.
- Components of one entity initialize in explicit dependency order where declared; otherwise use stable RTTI-ID order. A component may inspect sibling data after all components are constructed, but cannot assume an undeclared sibling has initialized runtime state.
- Shutdown reverses initialization order; deactivation reverses activation order. Release residency after all hooks and asynchronous uses requiring it have completed.

### Hierarchy readiness and publication

- Distinguish own component readiness from subtree readiness. An entity's subtree is loaded/initialized only when its own work and all attached children have reached those stages.
- Initial hierarchy initialization proceeds children-first. Hooks must not require initialized ancestors; they may read constructed ancestor data. Parent runtime services needed by children must be supplied by world services or an explicitly earlier preparation step.
- Registry roots are the activation entry points. Parents initiate child activation. Activate parent before children, stage publication, then expose the ready subtree atomically to queries at the safe point.
- Deactivate descendants before parents. Destroying a parent destroys its descendants by default. Detach/reparent children explicitly before destruction to keep them alive.
- Readiness gating applies to initial publication. Active parents remain active while newly added components or children load. Publish ready additions through their active entity/parent without reinitializing existing state.
- Ordinary queries match active rows and active required components only. Pending optional additions appear absent until activated. Adding a pending component must not hide an otherwise active entity from unrelated queries.
- Adding an initial child to a not-yet-active hierarchy extends the parent's readiness requirement. The commit owns the readiness accounting, so loading cannot race hierarchy edits.
- Maintain outstanding-child/transition counters instead of rescanning every hierarchy every frame.
- Keep minimal failure/cancellation bookkeeping in all builds. Development builds add detailed diagnostics. On failure/cancellation unwind completed stages in reverse order; never expose a partially activated initial subtree.
- Deactivation can retain initialized state/residency for later reactivation. Unload is a separate request that proceeds through shutdown and residency release. Destruction waits for safe teardown and outstanding jobs.

## 5. Command lists and structural publication

Introduce `EntityCommandList` with typed commands for create/destroy, add/remove component, set parent, rename, activation requests, registry unload, and component-value replacement. Retire `EntityDeferredAction` and per-entity deferred-action linked lists.

- Command payloads own their data in engine arenas. Support move-only values without `Rtti::Any` allocation per command/component. Record lists per executing job or caller; merge completed lists at a safe point.
- `CreateEntity` returns a list-local token. Subsequent commands can add components or reference parents created in that list. Tokens are not runtime IDs and cannot escape into persisted state.
- Targets that already exist use generation-validated runtime IDs. Stale targets yield command failure and diagnostics, never dereference freed addresses.
- Commands within one list preserve recording order. Across independent lists, order is unspecified. Detect conflicting operations on the same target rather than depending on worker completion order. Last-write-wins is allowed only within one list and documented commands.
- Coalesce each entity's component changes and allocate its final target archetype once. Validate the whole creation/hierarchy batch before publishing it.
- Creation eligibility is frame N+1 or later for any commands submitted in frame N. Additional commits in N cannot bypass this gate. Startup/bootstrap commands use a dedicated pre-simulation setup epoch.
- Before simulation in N+1, allocate and activate asset-free creations submitted in N. Start required asset requests early where possible; leave asset-dependent additions pending without blocking the frame.
- Destroy/remove/reparent/rename requests can apply at the next explicitly scheduled safe commit after their producers finish. New component/entity publication additionally obeys the frame eligibility gate.
- Queued-for-destruction entities stay valid until commit; command execution skips later incompatible operations according to documented list-order rules. Destruction invalidates the slot generation only after teardown finishes.
- Reject hierarchy cycles, cross-registry parenting, cross-world parenting, and invalid parent handles before modifying links.
- Intrusive sibling order is stable until explicit detach/reparent/destruction. Cascade queries guarantee parent-before-child completion, not stable sibling or general entity order.

## 6. Queries, phases, and scheduling

### Query terms

| Term | Presence | Access and callback value |
| --- | --- | --- |
| `const T` | Required active component | Read, `const T&` |
| `T` | Required active component | Read/write, `T&` |
| `const T*` | Optional active component | Read, nullable `const T*` |
| `T*` | Optional active component | Read/write, nullable `T*` |
| `Parent<const T>` | Required active immediate-parent component | Read, `const T&` |
| `Parent<const T*>` | Optional active immediate-parent component | Read, nullable `const T*` |

Only read-only parent-source terms are supported initially. Parent writes can alias between siblings and require a separate execution contract. Ordinary queries use exact component types, not inheritance matching. Validate duplicate/incompatible terms and callback signatures at compile time where possible. The optional leading `Entity&` callback parameter is auto-detected.

`Parent` means immediate parent, not nearest ancestor with the component. Missing, inactive, or root parent yields null for optional terms and excludes the entity for required terms. Parent-term access contributes to conflict detection even when that parent is not a self-match of the traversal.

Cache required/optional column mappings by archetype and invalidate matching caches when world chunk/archetype publication changes. Parent terms resolve the parent's current location during execution; do not persist component pointers in query caches.

### System and traversal API

Conceptual usage, with exact overload spelling finalized during Stage 1:

```cpp
void TransformationSystem::Update(Framework::EntityUpdateContext& ctx)
{
    using ResetQuery = Framework::Query<WorldTransformComponent>;
    auto reset = ResetQuery::Traverse(ctx, Framework::Phases::PreUpdate,
                                     [](WorldTransformComponent& output) { /* reset */ });

    using TransformQuery = Framework::CascadeQuery<
        const TransformComponent,
        const NonUniformScaleComponent*,
        Framework::Parent<const WorldTransformComponent*>,
        WorldTransformComponent>;

    TransformQuery::Traverse(ctx, Framework::Phases::PostUpdate, { reset },
                            [](const TransformComponent& local,
                               const NonUniformScaleComponent* scale,
                               const WorldTransformComponent* parent,
                               WorldTransformComponent& output) { /* compose */ });
}
```

- A traversal descriptor owns the callable, query metadata, phase, policy, prerequisites, originating system, and unsignaled completion group.
- Move/copy captures into an epoch-owned arena; reject/document dangling reference captures. System/context lifetime extends through all their scheduled work.
- The completion group covers all internal batches and post-execution bookkeeping. Keep an initial scheduling hold so it cannot signal before deferred job counts are established. Empty traversals complete when their scheduled phase/prerequisites permit them to complete.
- `WorldSystem::Update` runs once per update epoch to collect traversals. Collection is sequential initially and must not access live component data or wait on deferred traversal groups. Conditional component-driven behavior belongs inside scheduled callbacks.
- Init order is the application's explicit system list; Shutdown reverses it after all epoch work drains. System registration/removal occurs only at safe boundaries.
- Default sequential policy forbids overlapping callbacks originating from the same system, across all its traversals. This is callback serialization, not a simulation boundary or an implied semantic order.
- An explicit per-traversal parallel policy opts that traversal out of default system callback exclusion. The system author then owns synchronization of its state and any overlap with its other work.
- Conservative read/write conflict analysis initially operates at component-type scope across the world. Read/read can overlap; write/read and write/write cannot overlap. Optional and parent terms participate.
- Explicit prerequisites choose semantic order. Where a conflict has no explicit ordering, choose any acyclic safe orientation consistent with phase order and prerequisites. Do not define gameplay behavior through submission order.
- Build prerequisite edges first; add exclusion/conflict ordering without introducing cycles. Do not hold a system callback lock while waiting for another traversal from that system.

### Application-owned phase sequence

Declare phases with DrawTag-style macros and named runtime IDs. Do not copy DrawTag's fixed mask limit unless needed; phase identity does not encode order and is not a persistent serialized value.

```text
BeginFrame(frame N): apply eligible creation commands; publish ready loads
CollectWorldUpdates(epoch)
ScheduleUpdates(PreUpdate)
application stage or explicit structural commit
ScheduleUpdates(Update)
ScheduleUpdates(PostUpdate)
application render extraction / graphics scene processing
EndFrame: retain commands and pending loads for later frames
```

Every scheduled phase returns a completion group and depends on the preceding application stage's completion. Application stages must participate in this dependency chain, or explicitly wait before accessing shared state. A structural commit drains relevant phase work before modifying storage. Prefer a schedule-builder representation that can validate the ordered stages before dispatch.

- Detect phase duplication, omitted phases with queued work, cycles, and prerequisites from a later phase into an earlier one. Invalid schedules are rejected before dispatch.
- Allow opaque external prerequisite wait groups. Cycle/phase validation is complete for framework-owned groups; external producers are responsible for eventual completion and must not depend back on blocked framework work.
- Frame eligibility and update epoch are distinct: multiple fixed simulation ticks may have separate epochs in one rendered frame, but creation still waits for the next frame. A phase executes at most once per epoch.
- Implement component scheduling above existing `Jobs::Graph`/`JobNode`; do not dispatch collection work immediately through a graph that can start before the framework plan is validated.

### Parallel policies and fiber synchronization

Initial policies are Sequential, ParallelChunks for ordinary queries, and ParallelHierarchyTrees for cascade queries. ParallelHierarchyTrees runs each root subtree parent-first in one sequential task and permits independent subtrees to overlap. Batch several small trees into one task to avoid a job per entity. A cascade with a filtered-out parent still respects topology and declared parent access; it does not schedule that parent's missing callback implicitly.

Default sequential traversal also preserves parent-before-child callback completion. ParallelChunks is invalid for cascade queries unless later extended with explicit internal hierarchy dependencies.

Implement a Core `FiberMutex` with engine allocation, race-safe waiter registration, and fiber suspension/wakeup. It must not sleep the OS worker or spin for the duration of contention. Callers must not hold it while waiting for work that needs the same mutex. Do not require external-resource declarations in the first scheduler.

## 7. Change tracking and transforms

- Maintain monotonically increasing write versions for each chunk component column, with epoch-aware queries and consumer-owned last-observed versions. Handle counter wrap explicitly rather than relying on ordinary unsigned comparisons.
- A completed write traversal conservatively marks matching writable columns as changed, including false positives where callbacks did not actually write. Optional absent columns are not marked. Scope marking to columns/chunks included in that execution plan.
- Publish change marks before signaling traversal completion. Readers see completed writes through scheduler dependencies; submission alone does not mark changes.
- Newly activated components start changed. Archetype migration transfers change metadata conservatively so it cannot lose a pending change indication.
- Maintain hierarchy revisions in addition to component write versions. Reparenting marks all components of the moved entity changed; transform propagation also invalidates derived output throughout the moved subtree.
- Provide a changed filter with chunk-level granularity first. A consumer advances its observed version only after successfully processing the scheduled work. Independent consumers must not clear one another's changes.
- TransformationSystem observes local transforms, optional scale, hierarchy revisions, and changed parent world transforms. Process required descendants even when their own local components were unchanged. Mark derived world transforms changed for render consumers.

`SetParent` stores `ReparentMode` in the command payload. `PreserveWorld` is the default; `PreserveLocal` is explicit. The hierarchy core invokes a registered transform integration handler and contains no hard-coded transform component knowledge.

At commit, preserve the old world transform after preceding transform work completes, compute the candidate local transform against the new parent, and only then modify hierarchy links. If cached world data is stale, evaluate the needed ancestor chains before applying the operation. Validate the whole operation before mutation.

Use `FE::Transform` as authoritative local transform storage in `TransformComponent`; `WorldTransformComponent` is transient computed matrix output. `FE::Transform` contains translation, quaternion rotation, and uniform scale. `NonUniformScaleComponent` is an optional modifier composed before the local transform in the engine's row-vector convention. PreserveWorld decomposes the candidate local matrix into these authored values; reject results requiring shear, non-uniform scale without an existing modifier, or failed decomposition rather than silently losing information.

Reject PreserveWorld if the new parent's affine matrix is non-invertible; leave the hierarchy and components unchanged and report command failure. For entities without transform participation, SetParent performs topology-only mutation. A parent without a world-transform component contributes identity; require explicit transform components on any intermediate node that must transmit a transform. Initial tests establish multiplication/composition order rather than assuming the pseudocode's convention.

## 8. Assets, persistent instances, and references

### Asset types

| Type | Serialized responsibility | Runtime behavior |
| --- | --- | --- |
| `EntityCollection` | Source entity UUIDs, names, internal hierarchy, authored component payloads | Immutable reusable content; no simulation runtime state |
| `EntityCollectionInstanceAsset` | Persistent instance-root UUID, collection link, placement, source-to-concrete UUID bindings | Immutable placement description; streaming creates at most one membership in each world |
| `EntityWorldAsset` | Persistent world identity/settings, initial placement references, application-selected system configuration | Definition used to construct an explicitly owned runtime EntityWorld |
| `EntityWorldSnapshotAsset` | Full concrete entity/registry/hierarchy/component state and persistent placement bookkeeping | Restore runtime state with the same concrete UUIDs |

Assets are definition data, not mutable gameplay worlds. Asset finalization must not automatically add objects to a world. Application/world streaming code explicitly materializes them. The same definition can be used in separate runtime worlds; uniqueness is enforced within a world. Runtime registry IDs are never serialized; snapshot ownership groups use serialized keys mapped to newly created registries.

### Component payloads and serialization

- Collection and snapshot entity records contain UUID, name, parent UUID, and component records with RTTI type ID, serialization version/schema, and serialized authored data.
- Use generated serialization for concrete component fields. A focused manual serializer for the dynamic component envelope is allowed where generated reflection cannot express type-erased payloads.
- Cook payloads into compact buffers with offsets, not one allocated owning `Any` per live component. Deserialize directly into final chunk storage. Do not copy initialized runtime component state from assets.
- Enumerate nested asset references through actual serializers using the existing serialization-context callback. Preserve expected asset type and dependency kind. If cooked payloads are opaque bytes, store/replay the enumerated dependency metadata explicitly; otherwise the asset builder would miss their nested links.
- Add a dependency-only serialization sink first to avoid materializing redundant serialized byte buffers during runtime enumeration. Custom serializers must be side-effect-free and expose references through the same callback.
- Unknown component types or unsupported schema versions reject initial loading with diagnostics. Preserve-version migration and unknown-type round-tripping are follow-up features. Never use raw in-memory layouts as a disk contract.
- Register transient component/output policies so snapshots omit computed transforms, scene handles, loading state, runtime IDs, and query caches.

### Materialization modes and concrete UUIDs

- Runtime `SpawnCollection` creates fresh concrete UUIDs for every instance, remaps internal entity references, and can be repeated indefinitely.
- Authored placement load preserves the instance asset's stored concrete UUID bindings. Choose an explicit serialized `sourceEntityUuid -> entityUuid` table, rather than relying on a derived UUID algorithm.
- Give each placement a real instance-root entity whose UUID is the asset's instance UUID. Store its placement TransformComponent; attach all collection roots beneath it. The root is a normal queryable entity in the same registry, not a local system or simulation boundary.
- AssetBuilder/authoring code creates the binding table before runtime, validates uniqueness, retains bindings for unchanged source entities, and allocates new UUIDs for newly added source entities. Removed entities lose their membership; references to removed targets become unresolved.
- Copying a placement creates a new root UUID and new concrete bindings and remaps internal references. Saving/reloading an existing placement retains all IDs.
- A world tracks placement memberships by persistent instance UUID. Duplicate load returns the existing membership handle when it is the same placement; conflicting asset definitions for the same UUID fail. State includes pending/loading so concurrent requests cannot create duplicates.
- Copying an active hierarchy uses serialized authored values, not C++ copies of runtime handles; it is deferred creation with new UUIDs.
- Full snapshot save records current concrete entities, modifications, deletions, ownership groups, and hierarchy. Restore does not respawn the original collection first. This supports mutable instances without implementing property overrides.
- Capture snapshots at a safe boundary. Initially reject save while affected structural/lifecycle transitions are pending, with a precise diagnostic, instead of serializing partial state.

### Entity references

Introduce a distinct serialized `EntityReference` containing concrete entity UUID and, optionally, the owning placement asset's logical ID for diagnostics or explicit streaming requests. It is not an `IO::Link<Entity>` and contributes no automatic residency.

- Resolve against a supplied EntityWorld, returning a generation-checked runtime handle or null when the target is unloaded/destroyed. Default gameplay resolution returns only active targets; lifecycle code has an explicit allocated-target lookup.
- On stream-out, references remain serialized and unresolved. After stream-in, the same UUID can resolve again even though its runtime handle/address changed.
- Internal collection references remap to concrete UUIDs during materialization. External references keep their authored concrete UUIDs. Do not cache unresolved results permanently; world identity revisions invalidate lookups.
- References never force load or activation. Streaming/gameplay code explicitly requests the referenced placement asset when desired.
- Parent references are structural references with strict same-registry validation, not general soft references. Collections cannot introduce external parents; their roots attach through explicit materialization commands.

## 9. Residency and streaming

- Implement framework `EntityResidencySet` around independently owned `IO::AssetRequest` acquisitions. A registry has one shared set; ad hoc runtime-created entities acquire a lazily allocated entity-owned set. Components refer to a residency owner through transition bookkeeping.
- Deduplicate identical roots within an owner, retain expected-type constraints, and count component/entity contributions. Retain acquisitions for the whole transitive hard closure; do not substitute single-slot leases for closure ownership.
- Enumerate only hard links for automatic load/residency. Soft links remain gameplay/system-managed. Treat the current optional kind as non-hard until an explicit optional-loading policy is introduced.
- Materialization pins collection/placement definition generations while consuming their data. Acquire instance dependencies before releasing definition requests. Instances own their own component storage and residency; retaining the collection request is not their lifetime model.
- Initial serialized dependency metadata can accelerate loading, but component-level contribution tracking governs runtime removal and edits. A component replacing an asset link loads the replacement before publishing the value and releases the previous contribution after teardown/last use.
- Keep `AssetRead` generation pins only while accessing actual asset objects. Residency is not a permanent raw-pointer guarantee. Existing runtime renderer handles/leases follow graphics asset-generation rules.
- Registry stream-out stops new publication, cancels pending creation/load work, drains relevant jobs, deactivates descendants, shuts down/unloads components, and destroys entities/chunks before releasing final requests.
- No cross-registry hierarchy is allowed, so unloading a registry cannot leave dangling parent pointers elsewhere. General EntityReferences may become unresolved.
- Batch streaming by registry without adding registry-specific simulation phases or system updates. World queries always use the unified world index.
- Failed/pending streaming leaves existing active memberships unchanged. A pending load followed by unload cannot publish late entities; validate operation generation/cancellation tokens.
- Automatic propagation of reloaded collection/placement definitions is deferred. An explicit safe unload/reload uses the new definition while preserving authored instance bindings. Failed replacement leaves existing usable state intact where possible.

## 10. Implementation stages

Stages are ordered milestones, not promises about elapsed time. Each stage should be a reviewable change or small series of changes, preserve a buildable repository, and include only the validation needed for its new contracts.

### Stage 1 - API contract and framework test harness

Dependencies: none.

Work:

- Inventory old entity API consumers and identify the replacement surfaces. Current inspected consumers are concentrated in the entity implementation, but recheck samples/generated code when implementing.
- Finalize names/signatures for component contexts, lifecycle results, command lists/tokens, query terms, phase declarations, completion groups, policies, and asset records.
- Add a `Modules/Framework/Tests` GoogleTest executable `FeFrameworkTests`, following `FeCoreTests` setup, with scheduler/world fixture initialization and fake lifecycle/assets services. Register it with CTest and codegen where reflected test types require it.
- Provide small compile examples covering optional terms, optional Entity argument, Parent terms, multiple traversals, prerequisites, and application phase sequencing.
- Publish the ownership/threading contracts in the new public headers. Do not preserve obsolete APIs through permanent compatibility layers.

Acceptance:

- Framework test target builds and runs without a renderer/GPU.
- Public examples compile; invalid query signatures have focused diagnostics.
- Proposed framework-to-graphics dependency direction has no cycle.

### Stage 2 - RTTI move support and storage/identity replacement

Dependencies: Stage 1.

Work:

- Extend RTTI with optional move construction and no-throw capability metadata; add focused Core RTTI tests.
- Replace component IDs/duplicated generic metadata with RTTI-backed registration and dense internal indices.
- Implement immutable 64-bit entity IDs, world incarnation allocation, generation validation, UUID lookup, and stable entity pools.
- Rebuild aligned chunk layouts, canonical archetype matching, active metadata, compaction, migration, and memory release.
- Remove local EntitySystem/IComponentProvider and archetype notification machinery. Maintain temporary internal creation helpers for tests until command publication is implemented.

Acceptance:

- Nontrivial move-only components migrate with correct move/destruction counts and retained values.
- Over-aligned and oversized components are handled correctly; unsupported relocation is rejected.
- Entity pointers/IDs/UUIDs survive migration; stale handles and destroyed-world handles fail lookup.
- Archetype hash collisions cannot alias different layouts. Repeated create/migrate/destroy returns tracked allocations to baseline.

### Stage 3 - Command lists and hierarchy

Dependencies: Stage 2.

Work:

- Implement list-owned payloads, temporary entity tokens, batch validation/coalescing, and explicit safe commits.
- Add intrusive parent/children relationships, registry/world checks, cycle rejection, destroy-subtree behavior, and hierarchy revisions.
- Implement frame eligibility gates and bootstrap creation; remove old deferred-action allocators/lists.
- Add pending-addition metadata so unrelated queries can continue to see active entities during component loading.

Acceptance:

- Commands issued in frame N cannot create visible objects during any commit in N; asset-free objects are published before N+1 simulation.
- Lists can assemble whole hierarchies and components without per-component archetype migrations.
- Invalid/stale/cross-registry/cyclic operations leave hierarchy intact.
- Sibling lists remain stable across unrelated structural changes; subtree destruction clears every lookup and releases storage.

### Stage 4 - Lifecycle and residency

Dependencies: Stage 3.

Work:

- Bind detected stage callbacks and validate ordering/pairs. Implement own/subtree readiness counters, parent-driven activation, and reverse teardown.
- Implement dependency enumeration, owner residency sets, asynchronous pending load, failure/cancellation, and safe main-thread publication.
- Support additions/removals on active entities, deactivate/reactivate, unload, and registry teardown.
- Keep fake assets for deterministic tests; add integration tests against existing AssetManager fixtures where appropriate.

Acceptance:

- Hook traces verify stage ordering, bottom-up initial readiness, parent-first activation, and child-first teardown.
- Empty/no-asset components meet next-frame activation. Asset-dependent additions remain pending without hiding existing active components.
- Failure after each stage and cancellation during load unwind exactly once, do not publish late state, and release acquisitions.
- Shared dependencies survive removal of one contributor; soft links create no automatic acquisition.

### Stage 5 - Deferred queries and serial phase execution

Dependencies: Stages 3 and 4.

Work:

- Implement independent Query types, archetype/column caches, active filtering, optional arguments, and callback signature checking.
- Implement WorldSystem collection and Init/Shutdown, traversal records, completion holds, prerequisite retention, phase declaration macros, and schedule validation.
- Execute validated plans sequentially first while preserving deferred completion and application-stage dependencies.
- Add declared-access validation for component lookup through callback Entity arguments.

Acceptance:

- A system can submit multiple independent queries, including the example's reversed submission/phase order.
- Traverse performs no synchronous callbacks; empty traversals and dependent work complete correctly.
- Repeated/omitted phases, backward phase dependencies, and framework dependency cycles are rejected before execution.
- All registries participate equally; registry creation/removal updates query caches safely.

### Stage 6 - Parallel scheduler, cascade terms, and FiberMutex

Dependencies: Stage 5.

Work:

- Add read/write conflict edges and per-system default exclusion after explicit prerequisites.
- Implement ParallelChunks and batched ParallelHierarchyTrees over existing Core jobs/wait groups.
- Implement read-only Parent terms and topology-aware cascade matching, including ancestors not matched by self terms.
- Add FiberMutex to Core with deterministic contention/wakeup tests, rather than relying on timing-sensitive stress tests alone.
- Add scheduling/profiling diagnostics for component conflicts, work counts, and internal job batching.

Acceptance:

- Read/read work overlaps; conflicting work does not. Explicit prerequisites determine results independently of collection order.
- Callbacks from one default-sequential system never overlap, including across its traversals. Opted-in parallel work does overlap where safe.
- Parent callback completion precedes child execution; independent trees can overlap. Missing parent components obey required/optional semantics.
- Fiber contention releases workers to other work and has no lost wakeups or stranded waiters.
- Serial and parallel policies yield the same results for computations with declared ordering; no assertion depends on sibling iteration order.

### Stage 7 - Change tracking and transform integration

Dependencies: Stages 3, 5, and 6.

Work:

- Implement chunk-column write versions, independent changed consumers, activation/migration change handling, and hierarchy invalidation.
- Add reflected authored TransformComponent using FE::Transform, optional NonUniformScaleComponent, transient WorldTransformComponent, and TransformationSystem in GameFramework.
- Implement default PreserveWorld reparent command handler and explicit PreserveLocal.

Acceptance:

- Changes publish on completion; two consumers observe independently; migration cannot drop changes.
- Moving a parent updates all relevant descendant world transforms even with unchanged local values.
- Reparenting preserves representable world matrices and marks outputs changed; singular parents and unrepresentable local results reject the command list and preserve old state.
- Roots, missing transform parents, added children, deep chains, and many shallow trees follow the documented composition semantics.

### Stage 8 - Collection assets and persistent placements

Dependencies: Stages 4, 5, and 7.

Work:

- Implement collection/component envelopes and cooked buffers, per-component dependency metadata, asset registration, and AssetBuilder import.
- Implement repeated runtime spawn, persistent instance asset schema/binding validation, instance-root creation, internal reference remapping, and duplicate-membership protection.
- Add EntityReference serialization/resolution and operation tokens for asynchronous materialization.
- Release definition generation pins/requests after runtime data and dependency ownership are independent.

Acceptance:

- A collection can be spawned repeatedly with distinct UUIDs and released while all created groups remain alive and usable.
- Authored placements restore the same UUID bindings after stream-out/in and can be referenced before loading.
- Concurrent duplicate placement loads cannot create duplicate entities. Unknown types, invalid bindings, and unsupported schemas fail cleanly.
- Nested hard dependencies survive prototype release; soft links remain unmanaged. Internal references remap and external references remain unresolved/resolvable as appropriate.

### Stage 9 - World assets, snapshots, and streaming systems

Dependencies: Stage 8.

Work:

- Implement EntityWorldAsset definitions and explicit world construction with application-provided system factories/service bindings.
- Implement full concrete snapshot capture/restore, ownership-group keys, persistent placement bookkeeping, and UUID-preserving reload.
- Add a world streaming system in GameFramework that owns placement requests and runtime registries without adding simulation boundaries.
- Support registry load/unload requests and failed/canceled operation diagnostics.

Acceptance:

- Two runtime worlds can use the same world definition independently; runtime IDs cannot cross-resolve.
- Snapshot round-trip preserves UUIDs, names, hierarchy, authored component values, runtime modifications/deletions, and general entity references.
- Restoring a snapshot does not respawn source entities that were deleted in saved state.
- Stream-out followed by stream-in resolves references to new runtime handles with the same concrete UUIDs.
- Unload during pending load does not publish late entities or leak requests. Queries treat all loaded registries equally.

### Stage 10 - Graphics integration and usable sample

Dependencies: Stages 7 through 9.

Work:

- Add graphics mesh/camera components as needed and render world systems in GameFramework. Store scene instance handles as transient state; preserve component relocatability.
- Activation/deactivation creates/destroys graphics membership at safe points; scheduled sequential extraction updates changed transforms/material state.
- Order extraction after transformation completion and before existing graphics scene processing/render submission. Avoid renderer-side raw component pointers.
- Add a sample that loads a world with a persistent hierarchy, streams a second registry, repeatedly spawns an effect collection, reparents an entity, and saves/restores modified state.

Acceptance:

- Visible hierarchy transforms match world matrices and scene instances survive component migration.
- Removing/streaming-out entities removes graphics instances before residency release. No graphics handles appear in serialized snapshots.
- Unchanged chunks do not require transform resubmission; conservative false positives remain correct.
- The sample exercises application-defined/custom phase insertion and completes teardown without framework/graphics allocation leaks.

### Stage 11 - Performance verification and cleanup

Dependencies: Stage 10.

Work:

- Profile steady-state query traversal, change filtering, many shallow hierarchy trees, occasional deep chains, archetype churn, and registry stream-in/out.
- Measure allocations, chunk utilization, query-cache costs, job counts, and parent lookups. Tune chunk/tree batching based on evidence.
- Keep epoch arenas, cached column mappings, pooled entities/wait groups, and lazy residency owners. Do not allocate per entity per traversal.
- Verify all obsolete local systems/provider/component-ID/deferred-action APIs are gone, public documentation matches behavior, and generated files are current.
- Record benchmark baselines and remaining measured bottlenecks before introducing finer-grained conflict analysis or per-entity change tracking.

Acceptance:

- Fixed-content warmed-up updates have no per-entity heap allocations; frame/scheduler storage reuses reserved capacity or bounded arenas.
- World/registry teardown releases tracked framework allocations and residency, excluding documented process-lived manager caches.
- Fragmentation and job overhead have measured baselines. Optimization does not weaken identity, lifecycle, or ordering guarantees.

## 11. Repository workflow and validation

- Follow repository `AGENTS.md`, C++20, engine/festd allocators, and existing diagnostics. Use engine arenas/fixed or inline containers where they avoid common allocations; FiberTempAllocator is legal only inside fibers and must not back deferred data whose lifetime exceeds the fiber scope.
- Add new/removed source files to the relevant CMake source groups or confirm the existing configured glob includes them. Add the Framework tests subdirectory explicitly.
- Run `configure.bat` from the repository root after adding/removing files. Run `scripts/codegen.py` for changes to FE_RTTI classes and reflected asset/test types, including required regenerated output.
- Format modified C++ with `ThirdParty/llvm/clang-format.exe`; avoid vendored dependency churn.
- Build affected targets at each stage. Run focused Core RTTI/job/asset tests and Framework tests appropriate to the change. Run broader regression checks after integrated stages, not repeatedly when nothing changed.
- Use the documented one-shot process PATH-duplication workaround before Windows configuration/build/test invocations if required. Do not modify persistent environment settings.
- Test lifecycle/ordering with explicit barriers and hook traces. Do not infer concurrency correctness from sleep-based tests or randomized iteration alone.
- GPU-independent correctness stays in Framework tests. Graphics integration gets a sample/smoke validation and focused tests against fake scene submission where feasible.

## 12. First implementation milestone

Deliver Stages 1 through 5 first: RTTI-based movable chunk storage, stable entities and hierarchy, next-frame commands, complete lifecycle/residency, and deferred serial queries with explicit phase/prerequisite validation. This milestone establishes observable behavior before adding parallelism.

Then add parallel execution and transform tracking, followed by asset materialization/persistence and graphics integration. Persistent assets are implemented against the validated runtime contracts rather than freezing serialization around the old prototype.
