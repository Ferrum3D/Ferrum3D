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
        struct PendingCommands
        {
            festd::vector<EntityCommandList::Impl*> m_lists;
            Threading::SpinLock m_lock;
        };

        EntityWorld& m_owner;
        EntityStorage m_storage;
        EntityScheduler m_schedule;
        festd::vector<EntityMaterialization*> m_materializations;
        PendingCommands m_pendingCommands;
        EntityComponentRegistry m_components;
        ComponentLoadingState m_loading;
        EntityAssetServices* m_assets;
        void* m_services;
        ReparentHandler m_reparentHandler = nullptr;
        festd::ascii_view m_error;
        uint64_t m_nextRegistryId = 1;
        uint16_t m_token;
        bool m_started = false;

        Impl(EntityWorld& owner, EntityAssetServices* assets, void* services);
        ~Impl();
        EntityRegistry& CreateRegistry();
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

        bool Materialize(uint32_t operation);
        Entity* AllocateEntity(EntityRegistry& registry, Env::Name name, Uuid uuid);
        void DestroyEntity(Entity& entity);
        void Reparent(Entity& entity, Entity* parent);
        Archetype* GetArchetype(festd::span<const EntityComponentInfo* const> columns);
        void Migrate(Entity& entity, festd::span<const EntityComponentInfo* const> columns, festd::span<void* const> values);
        void AdvanceLifecycle();
        void PublishSubtree(Entity& entity);
        void UnloadSubtree(Entity& entity);
        void MarkUnready(Entity& entity);
        void UnwindSubtree(Entity& entity);

        bool PrepareSubtree(Entity& entity);
        bool ActivateSubtree(Entity& entity);
        void DeactivateSubtree(Entity& entity, bool keepAuthoredValues = true);
        void TeardownComponent(Entity& entity, uint32_t column, bool destroy);
        EntityResidencySet& ResidencyOwner(Entity& entity);
        void ReleaseAssets(Entity& entity, Rtti::TypeID component, uint64_t transition = 0);

        LifecycleResult LoadValue(Entity& entity, const EntityComponentInfo& info, void* data, ComponentStage& stage,
                                  uint64_t transition = 0);
        void TeardownValue(Entity& entity, const EntityComponentInfo& info, void* data, ComponentStage& stage,
                           uint64_t transition = 0);
        void AdvanceReplacements(Entity& entity);
        void CancelReplacements(Entity& entity, Rtti::TypeID type = Rtti::TypeID::kNull, bool keepAuthoredValues = false);

        bool RequireAsset(Entity& entity, IO::AssetID id, Rtti::TypeID type);
        EntityResources& GetResources(Entity& entity);
    };
} // namespace FE::Framework
