#pragma once
#include <Core/Threading/Fiber.h>
#include <Framework/Entities/EntityCommandListInternal.h>
#include <Framework/Entities/EntityMaterialization.h>
#include <Framework/Entities/EntityResources.h>
#include <Framework/Entities/EntityScheduler.h>
#include <Framework/Entities/EntityStorage.h>

namespace FE::Framework
{
    using Internal::CallbackContext;
    using Internal::Command;
    using Internal::CommandKind;

    struct EntityWorld::Impl
    {
        struct Transaction;

        struct PendingCommands
        {
            festd::vector<EntityCommandList::Impl*> m_lists;
            Threading::SpinLock m_lock;
        };


        struct LifecycleWork
        {
            festd::vector<EntityID> m_replacements;
            festd::vector<EntityID> m_roots;
        };

        EntityWorld& m_owner;
        EntityStorage m_storage;
        EntityScheduler m_schedule;
        festd::vector<EntityMaterialization*> m_materializations;
        festd::vector<uint32_t> m_activeMaterializations;
        festd::unordered_dense_map<IO::AssetID, uint32_t> m_placements;
        PendingCommands m_pendingCommands;
        EntityComponentRegistry m_components;
        ComponentLoadingState m_loading;
        LifecycleWork m_lifecycleWork;
        EntityAssetServices* m_assets;
        festd::vector<WorldService*> m_services;
        ReparentHandler m_reparentHandler = nullptr;
        festd::ascii_view m_error;
        // IDs are not recycled: stale EntityIDs and materialization tokens do not retain their world.
        uint16_t m_id;
        double m_fixedAccumulator = 0;
        bool m_started = false;
        uint64_t m_materializationBudgetEpoch = UINT64_MAX;
        uint64_t m_lifecycleBudgetEpoch = UINT64_MAX;
        uint32_t m_materializationsRemaining = 32;
        uint32_t m_lifecycleRemaining = 32;
        uint64_t m_materializationFrame = 0;
        bool m_explicitMaterializationFrames = false;

        Impl(EntityWorld& owner, EntityAssetServices* assets);
        ~Impl();
        EntityRegistry& CreateRegistry(Uuid key);
        void RemoveRegistry(EntityRegistry& registry);
        void* LookupComponent(const Entity& entity, Rtti::TypeID type, bool write) const;
        void Submit(EntityCommandList&& commands);
        MaterializationToken SpawnCollection(EntityRegistry& registry, const EntityCollection& collection);
        MaterializationToken SpawnCollection(EntityRegistry& registry, IO::AssetID collection);
        MaterializationToken LoadPlacement(EntityRegistry& registry, IO::AssetID placement);
        MaterializationToken LoadPlacement(EntityRegistry& registry, IO::AssetID placement,
                                           const EntityCollectionInstanceAsset& definition, const EntityCollection& collection);
        void CancelMaterialization(MaterializationToken token);

        bool Fail(festd::ascii_view message);
        void MarkChanged(Entity& entity);

        uint64_t NextChangeVersion();
        bool CommitImpl(bool bootstrap);
        void AdvanceMaterializations(bool bootstrap);

        bool Materialize(uint32_t operationIndex, uint32_t* budget = nullptr);
        bool LoadDefinition(const EntityWorldAsset& definition, festd::vector<MaterializationToken>* operations);
        bool CaptureSnapshot(EntityWorldSnapshotAsset& snapshot);
        bool RestoreSnapshot(const EntityWorldSnapshotAsset& snapshot, festd::vector<MaterializationToken>* operations);
        Entity* AllocateEntity(EntityRegistry& registry, Env::Name name, Uuid uuid);
        void DestroyEntity(Entity& entity);
        void Reparent(Entity& entity, Entity* parent);
        Archetype* GetArchetype(festd::span<const EntityComponentInfo* const> columns);
        void Migrate(Entity& entity, Archetype& archetype, festd::span<void* const> values);
        void AdvanceLifecycle();
        bool AdmitLifecycle(Entity& entity);
        void PublishSubtree(Entity& entity);
        void PublishAdditions(Entity& entity);
        void UnloadSubtree(Entity& entity);
        void MarkUnready(Entity& entity);
        void UnwindSubtree(Entity& entity);

        bool PrepareSubtree(Entity& entity);
        LifecycleResult ActivateSubtree(Entity& entity);
        void DeactivateSubtree(Entity& entity, bool keepAuthoredValues = true);
        void DeactivateEntity(Entity& entity, bool keepAuthoredValues);
        void TeardownComponent(Entity& entity, uint32_t column, bool destroy);
        EntityResidencySet& ResidencyOwner(Entity& entity);
        void ReleaseAssets(Entity& entity, Rtti::TypeID component, uint64_t transition = 0);

        LifecycleResult LoadValue(Entity& entity, const EntityComponentInfo& info, void* data, ComponentStage& stage,
                                  uint64_t transition = 0);
        void TeardownValue(Entity& entity, const EntityComponentInfo& info, void* data, ComponentStage& stage,
                           uint64_t transition = 0);
        void AdvanceReplacements(Entity& entity);
        void CancelReplacements(Entity& entity, Rtti::TypeID type = Rtti::TypeID::kNull, bool keepAuthoredValues = false);

        void RequireAsset(Entity& entity, IO::AssetID id, Rtti::TypeID type);
        EntityResources& GetResources(Entity& entity);
    };
} // namespace FE::Framework
