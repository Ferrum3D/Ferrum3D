#pragma once
#include <Framework/Entities/Archetype.h>
#include <Framework/Entities/Entity.h>
#include <Framework/Entities/EntityCommandList.h>
#include <Framework/Entities/EntityRegistry.h>
#include <Framework/Entities/EntityUpdateContext.h>

namespace FE::Framework
{
    struct QueryAccess final
    {
        Rtti::TypeID m_type;
        bool m_optional = false;
        bool m_write = false;
        bool m_parent = false;
    };


    // All world mutation, lifecycle hooks, collection and serial phase execution occur at main-thread safe points.
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
        // Bootstrap ends permanently with the first BeginUpdate. Normal lists never bypass their next-frame eligibility.
        bool CommitBootstrap();
        bool Commit();
        void BeginUpdate();
        void AddSystem(WorldSystem& system);
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
        [[nodiscard]] uint64_t GetHierarchyRevision() const;
        [[nodiscard]] festd::ascii_view GetLastError() const;
        [[nodiscard]] uint32_t GetEntityCount() const;
        [[nodiscard]] uint32_t GetChunkCount() const;
        // Query callbacks are arena-owned until EndUpdate. Captured references must outlive the epoch.
        Rc<WaitGroup> RecordTraversal(EntityUpdateContext& context, Phase phase, festd::span<const QueryAccess> accesses,
                                      void* callable, void (*invoke)(void*, Entity&, void**), void (*destroy)(void*),
                                      festd::span<const Rc<WaitGroup>> prerequisites,
                                      ExecutionPolicy policy = ExecutionPolicy::kSequential);
        void* AllocateTraversal(size_t size, size_t alignment);
        [[nodiscard]] void* LookupComponent(const Entity& entity, Rtti::TypeID type, bool write) const;
        bool RequireAsset(Entity& entity, IO::AssetID id, Rtti::TypeID type);

    private:
        struct Impl;
        Impl* m_impl;
        bool Fail(festd::ascii_view message);
        Rc<WaitGroup> RecordStage(Phase stage, void* callable, void (*invoke)(void*), void (*destroy)(void*),
                                  festd::span<const Rc<WaitGroup>> prerequisites);
        bool CommitImpl(bool bootstrap);
        Entity* AllocateEntity(EntityRegistry& registry, Env::Name name, Uuid uuid);
        void DestroyEntity(Entity& entity);
        void Reparent(Entity& entity, Entity* parent);
        void Migrate(Entity& entity, festd::span<const EntityComponentInfo* const> columns, festd::span<void* const> values);
        void AdvanceLifecycle();
        void MarkUnready(Entity& entity);
        void UnwindSubtree(Entity& entity);
        bool PrepareSubtree(Entity& entity);
        bool ActivateSubtree(Entity& entity);
        void DeactivateSubtree(Entity& entity);
        void TeardownComponent(Entity& entity, uint32_t column, bool destroy);
        EntityResidencySet& ResidencyOwner(Entity& entity);
        void ReleaseAssets(Entity& entity, Rtti::TypeID component, uint64_t transition = 0);
        LifecycleResult LoadValue(Entity& entity, const EntityComponentInfo& info, void* data, uint8_t& stage,
                                  uint64_t transition = 0);
        void TeardownValue(Entity& entity, const EntityComponentInfo& info, void* data, uint8_t& stage, uint64_t transition = 0);
        void AdvanceReplacements(Entity& entity);
        void CancelReplacements(Entity& entity, Rtti::TypeID type = Rtti::TypeID::kNull);
    };
} // namespace FE::Framework
