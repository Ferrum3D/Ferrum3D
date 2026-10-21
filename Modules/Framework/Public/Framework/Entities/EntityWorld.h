#pragma once
#include <Framework/Entities/Archetype.h>
#include <Framework/Entities/Entity.h>
#include <Framework/Entities/EntityCollection.h>
#include <Framework/Entities/EntityCommandList.h>
#include <Framework/Entities/EntityRegistry.h>
#include <Framework/Entities/EntityReparent.h>
#include <Framework/Entities/EntityUpdateContext.h>
#include <Framework/Entities/WorldService.h>

namespace FE::Framework
{
    //! @brief Persistent traversal snapshot; initially zero to request a first complete traversal.
    struct ChangeCursor final
    {
        //! @brief Last consumed change version.
        uint64_t m_version = 0;
        //! @brief Distinguish change-version rollover from an unchanged snapshot.
        uint64_t m_versionEra = 0;
        //! @brief Last consumed parent-link revision.
        uint64_t m_hierarchyRevision = 0;
        //! @brief Last consumed component-layout revision.
        uint64_t m_structureRevision = 0;
        //! @brief Last consumed entity/component publication serial, independent of ordinary writes.
        uint64_t m_additionVersion = 0;
    };


    //! @brief Select published additions since a traversal's persistent cursor was last consumed.
    enum class QueryFilter : uint8_t
    {
        kAll,
        //! @brief Entities published for the first time; reactivation does not count.
        kNewEntities,
        //! @brief Any queried local component published for the first time, including optional terms.
        //! Replacements and retained components do not count; parent and Without terms only constrain matching.
        kAddedComponents
    };


    //! @brief Component exclusion edge between traversal indices in the current epoch.
    struct ScheduleConflict final
    {
        Rtti::TypeID m_component;
        //! @brief Traversal index that must complete first.
        uint32_t m_beforeTraversal;
        //! @brief Traversal index delayed by this exclusion edge.
        uint32_t m_afterTraversal;
    };


    //! @brief Counters accumulated by the most recent scheduled epoch.
    struct ScheduleDiagnostics final
    {
        //! @brief Number of component-access exclusion edges.
        uint32_t m_conflictEdges = 0;
        //! @brief Number of exclusions for sequential work from one system.
        uint32_t m_systemEdges = 0;
        //! @brief Number of exclusions protecting application stages.
        uint32_t m_stageEdges = 0;
        //! @brief Number of dispatched traversal and subdivision jobs.
        uint32_t m_jobs = 0;
        //! @brief Number of matched chunks processed.
        uint32_t m_chunks = 0;
        //! @brief Number of dispatched hierarchy-level batches.
        uint32_t m_treeBatches = 0;
        //! @brief Number of invoked entity callbacks.
        uint32_t m_callbacks = 0;
    };


    //! @brief Type-erased component access declaration used for matching and race exclusion.
    struct QueryAccess final
    {
        Rtti::TypeID m_type;
        //! @brief Permit a missing component and pass null to the callback.
        bool m_optional = false;
        //! @brief Declare mutation; excludes overlapping component access.
        bool m_write = false;
        //! @brief Resolve this term on the immediate active parent.
        bool m_parent = false;
        //! @brief Reject archetypes containing this component; does not declare a data access.
        bool m_excluded = false;
    };


    //! @brief Invoke one typed callback with a matched entity and its component addresses.
    using TraversalInvoke = bool (*)(void*, Entity&, void**);

    //! Return this explicitly from a query callback to opt into conditional write publication.
    enum class QueryWriteResult
    {
        kUnchanged,
        kChanged
    };
    //! @brief Invoke a no-argument main-thread application stage.
    using StageInvoke = void (*)(void*);
    //! @brief Destroy a callback object without freeing its epoch-owned storage.
    using CallableDestroy = void (*)(void*);


    //! @brief Type-erased traversal request; spans are copied and callback ownership transfers to the epoch.
    struct TraversalDesc final
    {
        //! @brief Phase containing this work.
        Phase m_phase;
        //! @brief Borrowed component access declarations, copied when recorded.
        festd::span<const QueryAccess> m_accesses;
        //! @brief Owned type-erased callback storage released at epoch end.
        void* m_callable = nullptr;
        //! @brief Callback invocation adapter.
        TraversalInvoke m_invoke = nullptr;
        //! @brief Callback destructor adapter; storage itself belongs to the epoch arena.
        CallableDestroy m_destroy = nullptr;
        //! @brief Completion groups that must finish before execution.
        festd::span<const Rc<WaitGroup>> m_prerequisites;
        //! @brief Optional persistent change cursor; must outlive the epoch.
        ChangeCursor* m_cursor = nullptr;
        ExecutionPolicy m_policy = ExecutionPolicy::kSequential;
        //! @brief Visit active parents before descendants.
        bool m_cascade = false;
        QueryFilter m_filter = QueryFilter::kAll;
    };


    //! @brief Type-erased main-thread application stage; callback storage must belong to the current epoch.
    struct StageDesc final
    {
        //! @brief Phase containing this work.
        Phase m_phase;
        //! @brief Owned type-erased callback storage released at epoch end.
        void* m_callable;
        //! @brief Callback invocation adapter.
        StageInvoke m_invoke;
        //! @brief Callback destructor adapter; storage itself belongs to the epoch arena.
        CallableDestroy m_destroy;
        //! @brief Completion groups that must finish before execution.
        festd::span<const Rc<WaitGroup>> m_prerequisites;
    };


    //! @brief Own entities, storage, and a shared schedule across all registries; safe points require the main thread.
    //! Borrowed asset services must outlive the world.
    //! World mutation, lifecycle hooks and collection occur at main-thread safe points. Queries use job fibers;
    //! kMainThread keeps query callbacks on main-thread fibers.
    //! Application stages may pass external completion groups. Use --ecs-validation to check the full schedule before execution.
    struct EntityWorld final
    {
        explicit EntityWorld(EntityAssetServices* assets = nullptr);
        ~EntityWorld();

        EntityWorld(const EntityWorld&) = delete;
        EntityWorld& operator=(const EntityWorld&) = delete;

        //! @brief Resolve a runtime ID or UUID; return null for stale, foreign, absent, or filtered inactive entities.
        [[nodiscard]] Entity* Find(EntityID id) const;
        [[nodiscard]] Entity* Find(Uuid uuid, bool activeOnly = true) const;

        //! @brief Create a world-owned residency group at a main-thread safe point.
        EntityRegistry& CreateRegistry(Uuid key = Uuid::kNull);
        //! @brief Find a live ownership group by persistent key.
        [[nodiscard]] EntityRegistry* FindRegistry(Uuid key) const;
        //! @brief Load concrete entities without remapping UUIDs or creating an extra root.
        MaterializationToken LoadEntities(EntityRegistry& registry, const EntityCollection& entities);
        //! @brief Load a definition into an empty world after application systems have registered its component types.
        bool LoadDefinition(const EntityWorldAsset& definition, festd::vector<MaterializationToken>* operations = nullptr);
        //! @brief Capture settled concrete values; leaves output untouched on failure.
        bool CaptureSnapshot(EntityWorldSnapshotAsset& snapshot) const;
        //! @brief Restore into an empty world; transient columns are regenerated from registered companions.
        bool RestoreSnapshot(const EntityWorldSnapshotAsset& snapshot, festd::vector<MaterializationToken>* operations = nullptr);
        //! @brief Cancel its pending work and destroy its entities; the registry reference becomes invalid.
        void RemoveRegistry(EntityRegistry& registry);
        //! @brief Remove all registries and cancel pending materializations while retaining system registrations.
        void Clear();

        //! @brief Return the world-owned registry; register policies before component instances are created.
        EntityComponentRegistry& Components();
        //! @brief Transfer command ownership; creations and component edits become eligible next epoch.
        void Submit(EntityCommandList&& commands);

        //! @brief Queue a collection with fresh UUIDs beneath a generated root; content failures are reported by its token.
        MaterializationToken SpawnCollection(EntityRegistry& registry, const EntityCollection& collection);
        MaterializationToken SpawnCollection(EntityRegistry& registry, IO::AssetID collection);
        //! @brief Queue an authored placement, preserving bindings; repeated pending or ready asset requests share a token.
        MaterializationToken LoadPlacement(EntityRegistry& registry, IO::AssetID placement);
        MaterializationToken LoadPlacement(EntityRegistry& registry, IO::AssetID placement,
                                           const EntityCollectionInstanceAsset& definition, const EntityCollection& collection);

        //! @brief Return publication state, root ID, and failure reason; foreign tokens return a failed status.
        [[nodiscard]] MaterializationStatus GetMaterializationStatus(MaterializationToken token) const;
        //! @brief Borrow the operation UUID table until cancellation, advancement, or world destruction.
        [[nodiscard]] festd::span<const EntityUuidBinding> GetMaterializationBindings(MaterializationToken token) const;

        //! @brief Cancel definition loads and destroy generated membership at a main-thread safe point.
        void CancelMaterialization(MaterializationToken token);

        //! @brief Commit startup work immediately, before the first update; normal command lists retain next-frame eligibility.
        bool CommitBootstrap();
        //! @brief Commit eligible batches at a safe point; return false if a transaction is rejected.
        bool Commit();

        //! @brief Advance the epoch, commit eligible changes, and let systems record work on the main thread.
        void BeginUpdate();
        //! @brief Collect system FixedUpdate hooks without ticking frame services; schedule phases independently of frequency.
        void BeginFixedUpdate(float deltaSeconds);
        //! @brief Accumulate frame time and execute fixed steps; preserve fractional time and limit catch-up work.
        template<class Callable>
        bool RunFixedUpdates(double deltaSeconds, Callable&& schedule, float fixedDeltaSeconds = 1.0f / 60.0f,
                             uint32_t maxSteps = 8)
        {
            static_assert(std::is_invocable_r_v<void, Callable&, EntityWorld&>);

            auto callback = [&schedule](EntityWorld& world) {
                schedule(world);
            };

            return RunFixedUpdates(
                deltaSeconds,
                fixedDeltaSeconds,
                maxSteps,
                [](void* callable, EntityWorld& world) {
                    (*static_cast<decltype(callback)*>(callable))(world);
                },
                &callback);
        }
        //! @brief Borrow a system and initialize it; registration requires a closed update epoch.
        void AddSystem(WorldSystem& system);
        //! @brief Install one transactional reparent handler; clearing it requires a safe point.
        void SetReparentHandler(ReparentHandler handler);
        //! @brief Shut down and detach a borrowed system outside an update epoch.
        void RemoveSystem(WorldSystem& system);

        //! @brief Borrow and initialize a service at a closed main-thread boundary; it must outlive registration.
        void AddService(WorldService& service);
        //! @brief Shut down and detach a service after clearing entities, outside an update epoch.
        void RemoveService(WorldService& service);

        //! @brief Find a registered service by reflected type or base; the pointer is borrowed until removal.
        template<class T>
            requires std::derived_from<T, WorldService>
        [[nodiscard]] T* FindService() const
        {
            return static_cast<T*>(FindService(Rtti::GetTypeID<T>()));
        }

        //! @brief Append one phase to execution order; duplicate phases and closed epochs are programmer errors.
        bool SchedulePhase(Phase phase, festd::span<const Rc<WaitGroup>> prerequisites = {});

        //! @brief Own a no-argument main-thread callback until EndUpdate; captured references must outlive the epoch.
        template<class Callable>
        Rc<WaitGroup> ScheduleStage(Phase stage, Callable&& callable, festd::span<const Rc<WaitGroup>> prerequisites = {})
        {
            if (!SchedulePhase(stage))
                return {};

            return RecordStage(stage, std::forward<Callable>(callable), prerequisites);
        }


        //! @brief Record main-thread work in an application-ordered phase without scheduling that phase implicitly.
        template<class Callable>
        Rc<WaitGroup> RecordStage(Phase stage, Callable&& callable, festd::span<const Rc<WaitGroup>> prerequisites = {})
        {
            using Function = std::decay_t<Callable>;
            static_assert(std::is_invocable_r_v<void, Function&>, "Stage callback takes no arguments");

            void* storage = AllocateTraversal(sizeof(Function), alignof(Function));
            ::new (storage) Function(std::forward<Callable>(callable));
            return RecordStage({ stage,
                                 storage,
                                 [](void* function) {
                                     (*static_cast<Function*>(function))();
                                 },
                                 [](void* function) {
                                     static_cast<Function*>(function)->~Function();
                                 },
                                 prerequisites });
        }

        //! @brief Order a recorded completion after another group; completion must belong to the open epoch.
        bool AddPrerequisite(WaitGroup& completion, const Rc<WaitGroup>& prerequisite);
        //! @brief Explicitly validate phases, query policies, and dependencies; invalid plans assert even in Release.
        bool ValidateSchedule();
        //! @brief Compile and execute the plan, validating first when --ecs-validation is enabled.
        bool ExecuteSchedule();
        //! @brief Release callback captures and close the epoch; unfinished recorded work is a programmer error.
        bool EndUpdate();

        //! @brief Reset materialization/lifecycle quotas once per rendered frame, shared by all its world updates.
        //! Standalone worlds that omit this call receive a fresh quota per BeginUpdate.
        void BeginFrame();
        [[nodiscard]] uint64_t GetEpoch() const;
        //! @brief Return a synchronized copy of counters for the most recent update plan.
        [[nodiscard]] ScheduleDiagnostics GetScheduleDiagnostics() const;
        //! @brief Borrow component exclusion edges until the next BeginUpdate or world destruction.
        [[nodiscard]] festd::span<const ScheduleConflict> GetScheduleConflicts() const;
        //! @brief Return the revision incremented when parent links or entity membership change.
        [[nodiscard]] uint64_t GetHierarchyRevision() const;
        //! @brief Return the latest recoverable transaction or content failure; the next commit clears it.
        [[nodiscard]] festd::ascii_view GetLastError() const;
        //! @brief Return allocated entity count, including inactive and loading entities.
        [[nodiscard]] uint32_t GetEntityCount() const;
        [[nodiscard]] uint32_t GetChunkCount() const;

        //! @brief Own the described callback until EndUpdate; spans are copied and the context must name the open epoch.
        //! Captured references must outlive the epoch.
        Rc<WaitGroup> RecordTraversal(EntityUpdateContext& context, const TraversalDesc& desc);

        //! @brief Allocate callback storage for the open epoch; storage remains valid until EndUpdate.
        void* AllocateTraversal(size_t size, size_t alignment);
        //! @brief Borrow a component until structural commit; query callbacks must declare the requested access.
        [[nodiscard]] void* LookupComponent(const Entity& entity, Rtti::TypeID type, bool write) const;

        //! @brief Contribute a hard dependency during a component loading hook; null IDs are optional.
        void RequireAsset(Entity& entity, IO::AssetID id, Rtti::TypeID type);

    private:
        friend struct EntityCommandList;
        friend struct EntityLifecycle;
        friend struct EntityMaterializer;
        friend struct EntityScheduler;

        struct Impl;
        Impl* m_impl;

        Rc<WaitGroup> RecordStage(const StageDesc& desc);
        void* FindService(Rtti::TypeID type) const;
        bool RunFixedUpdates(double deltaSeconds, float fixedDeltaSeconds, uint32_t maxSteps,
                             void (*schedule)(void*, EntityWorld&), void* callable);
    };
} // namespace FE::Framework
