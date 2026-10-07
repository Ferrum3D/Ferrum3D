#pragma once
#include <Framework/Entities/Archetype.h>
#include <Framework/Entities/Entity.h>
#include <Framework/Entities/EntityCollection.h>
#include <Framework/Entities/EntityCommandList.h>
#include <Framework/Entities/EntityRegistry.h>
#include <Framework/Entities/EntityReparent.h>
#include <Framework/Entities/EntityUpdateContext.h>

namespace FE::Framework
{
    struct ChangeCursor final
    {
        uint64_t m_version = 0;
        uint64_t m_versionEra = 0;
        uint64_t m_hierarchyRevision = 0;
        uint64_t m_structureRevision = 0;
    };


    struct ScheduleConflict final
    {
        Rtti::TypeID m_component;
        uint32_t m_beforeTraversal;
        uint32_t m_afterTraversal;
    };


    struct ScheduleDiagnostics final
    {
        uint32_t m_conflictEdges = 0;
        uint32_t m_systemEdges = 0;
        uint32_t m_stageEdges = 0;
        uint32_t m_jobs = 0;
        uint32_t m_chunks = 0;
        uint32_t m_treeBatches = 0;
        uint32_t m_callbacks = 0;
    };


    struct QueryAccess final
    {
        Rtti::TypeID m_type;
        bool m_optional = false;
        bool m_write = false;
        bool m_parent = false;
    };


    // World mutation, lifecycle hooks and collection occur at main-thread safe points. Query callbacks execute on job fibers.
    // Application stages may pass external completion groups. Validate the full phase plan before executing any callbacks.
    struct EntityWorld final
    {
        explicit EntityWorld(EntityAssetServices* assets = nullptr, void* services = nullptr);
        ~EntityWorld();

        EntityWorld(const EntityWorld&) = delete;
        EntityWorld& operator=(const EntityWorld&) = delete;

        [[nodiscard]] Entity* Find(EntityID id) const;
        [[nodiscard]] Entity* Find(Uuid uuid, bool activeOnly = true) const;

        EntityRegistry& CreateRegistry();
        void RemoveRegistry(EntityRegistry& registry);

        EntityComponentRegistry& Components();
        void Submit(EntityCommandList&& commands);

        MaterializationToken SpawnCollection(EntityRegistry& registry, const EntityCollection& collection);
        MaterializationToken SpawnCollection(EntityRegistry& registry, IO::AssetID collection);
        MaterializationToken LoadPlacement(EntityRegistry& registry, IO::AssetID placement);
        MaterializationToken LoadPlacement(EntityRegistry& registry, IO::AssetID placement,
                                           const EntityCollectionInstanceAsset& definition, const EntityCollection& collection);

        [[nodiscard]] MaterializationStatus GetMaterializationStatus(MaterializationToken token) const;
        [[nodiscard]] festd::span<const EntityUuidBinding> GetMaterializationBindings(MaterializationToken token) const;

        void CancelMaterialization(MaterializationToken token);

        // Bootstrap ends permanently with the first BeginUpdate. Normal lists never bypass their next-frame eligibility.
        bool CommitBootstrap();
        bool Commit();

        void BeginUpdate();
        void AddSystem(WorldSystem& system);
        void SetReparentHandler(ReparentHandler handler);
        void RemoveSystem(WorldSystem& system);
        bool SchedulePhase(Phase phase, festd::span<const Rc<WaitGroup>> prerequisites = {});

        template<class Callable>
        Rc<WaitGroup> ScheduleStage(Phase stage, Callable&& callable, festd::span<const Rc<WaitGroup>> prerequisites = {})
        {
            using Function = std::decay_t<Callable>;
            static_assert(std::is_invocable_r_v<void, Function&>, "Application stage callback takes no arguments");
            if (!SchedulePhase(stage))
                return {};

            void* storage = AllocateTraversal(sizeof(Function), alignof(Function));
            ::new (storage) Function(std::forward<Callable>(callable));
            return RecordStage(
                stage,
                storage,
                [](void* function) {
                    (*static_cast<Function*>(function))();
                },
                [](void* function) {
                    static_cast<Function*>(function)->~Function();
                },
                prerequisites);
        }

        bool AddPrerequisite(WaitGroup& completion, const Rc<WaitGroup>& prerequisite);
        bool ValidateSchedule();
        bool ExecuteSchedule();
        bool EndUpdate();

        [[nodiscard]] uint64_t GetEpoch() const;
        [[nodiscard]] ScheduleDiagnostics GetScheduleDiagnostics() const;
        [[nodiscard]] festd::span<const ScheduleConflict> GetScheduleConflicts() const;
        [[nodiscard]] uint64_t GetHierarchyRevision() const;
        [[nodiscard]] festd::ascii_view GetLastError() const;
        [[nodiscard]] uint32_t GetEntityCount() const;
        [[nodiscard]] uint32_t GetChunkCount() const;

        // Query callbacks are arena-owned until EndUpdate. Captured references must outlive the epoch.
        Rc<WaitGroup> RecordTraversal(EntityUpdateContext& context, Phase phase, festd::span<const QueryAccess> accesses,
                                      void* callable, void (*invoke)(void*, Entity&, void**), void (*destroy)(void*),
                                      festd::span<const Rc<WaitGroup>> prerequisites,
                                      ExecutionPolicy policy = ExecutionPolicy::kSequential, bool cascade = false,
                                      ChangeCursor* cursor = nullptr);

        void* AllocateTraversal(size_t size, size_t alignment);
        [[nodiscard]] void* LookupComponent(const Entity& entity, Rtti::TypeID type, bool write) const;
        bool RequireAsset(Entity& entity, IO::AssetID id, Rtti::TypeID type);

    private:
        struct Impl;
        Impl* m_impl;

        bool Fail(festd::ascii_view message);
        Rc<WaitGroup> RecordStage(Phase stage, void* callable, void (*invoke)(void*), void (*destroy)(void*),
                                  festd::span<const Rc<WaitGroup>> prerequisites);
        void ExecuteTraversal(uint32_t index);
        void MarkChanged(Entity& entity);
        uint64_t NextChangeVersion();
        bool CommitImpl(bool bootstrap);
        void AdvanceMaterializations(bool bootstrap);
        bool Materialize(uint32_t operation);
        Entity* AllocateEntity(EntityRegistry& registry, Env::Name name, Uuid uuid);
        void DestroyEntity(Entity& entity);
        void Reparent(Entity& entity, Entity* parent);
        void Migrate(Entity& entity, festd::span<const EntityComponentInfo* const> columns, festd::span<void* const> values);
        void AdvanceLifecycle();
        void MarkUnready(Entity& entity);
        void UnwindSubtree(Entity& entity);
        bool PrepareSubtree(Entity& entity);
        bool ActivateSubtree(Entity& entity);
        void DeactivateSubtree(Entity& entity, bool keepAuthoredValues = true);
        void TeardownComponent(Entity& entity, uint32_t column, bool destroy);
        EntityResidencySet& ResidencyOwner(Entity& entity);
        void ReleaseAssets(Entity& entity, Rtti::TypeID component, uint64_t transition = 0);
        LifecycleResult LoadValue(Entity& entity, const EntityComponentInfo& info, void* data, uint8_t& stage,
                                  uint64_t transition = 0);
        void TeardownValue(Entity& entity, const EntityComponentInfo& info, void* data, uint8_t& stage, uint64_t transition = 0);
        void AdvanceReplacements(Entity& entity);
        void CancelReplacements(Entity& entity, Rtti::TypeID type = Rtti::TypeID::kNull, bool keepAuthoredValues = false);
    };
} // namespace FE::Framework
