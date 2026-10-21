#pragma once
#include <Core/Threading/Fiber.h>
#include <Framework/Entities/EntityCommandListInternal.h>
#include <Framework/Entities/EntityLifecycle.h>
#include <Framework/Entities/EntityMaterializer.h>
#include <Framework/Entities/EntityScheduler.h>
#include <Framework/Entities/EntityStorage.h>

namespace FE::Framework
{
    using Internal::CallbackContext;
    using Internal::Command;
    using Internal::CommandKind;

    //! @brief World implementation: entity storage and structural edits. Scheduling, component lifecycle and
    //! materialization are delegated to their own members; every method runs at a main-thread safe point.
    struct EntityWorld::Impl
    {
        struct Transaction;

        struct PendingCommands
        {
            festd::vector<EntityCommandList::Impl*> m_lists;
            Threading::SpinLock m_lock;
        };

        EntityWorld& m_owner;
        EntityComponentRegistry m_components;
        EntityStorage m_storage;
        EntityScheduler m_schedule;
        EntityLifecycle m_lifecycle;
        EntityMaterializer m_materializer;
        PendingCommands m_pendingCommands;
        EntityAssetServices* m_assets;
        festd::vector<WorldService*> m_services;
        ReparentHandler m_reparentHandler = nullptr;
        festd::ascii_view m_error;
        // IDs are not recycled: stale EntityIDs and materialization tokens do not retain their world.
        uint16_t m_id;
        double m_fixedAccumulator = 0;
        bool m_started = false;
        //! @brief Rendered-frame counter for shared quotas; unused until the application calls BeginFrame.
        uint64_t m_frame = 0;
        bool m_explicitFrames = false;

        Impl(EntityWorld& owner, EntityAssetServices* assets);
        ~Impl();

        EntityRegistry& CreateRegistry(Uuid key);
        void RemoveRegistry(EntityRegistry& registry);
        [[nodiscard]] bool OwnsRegistry(const EntityRegistry& registry) const;

        Entity* AllocateEntity(EntityRegistry& registry, Env::Name name, Uuid uuid);
        void DestroyEntity(Entity& entity);
        void Reparent(Entity& entity, Entity* parent);
        Archetype* GetArchetype(festd::span<const EntityComponentInfo* const> columns);
        //! @brief Move an entity into a layout; non-null values replace columns and active columns are staged.
        void Migrate(Entity& entity, Archetype& archetype, festd::span<void* const> values);

        uint64_t NextChangeVersion();
        void MarkChanged(Entity& entity);
        void* LookupComponent(const Entity& entity, Rtti::TypeID type, bool write) const;

        void Submit(EntityCommandList&& commands);
        bool CommitImpl(bool bootstrap);
        bool Fail(festd::ascii_view message);
        //! @brief Return the frame that keys lifecycle and materialization quotas.
        [[nodiscard]] uint64_t GetQuotaFrame() const;

        bool LoadDefinition(const EntityWorldAsset& definition, festd::vector<MaterializationToken>* operations);
        bool CaptureSnapshot(EntityWorldSnapshotAsset& snapshot);
        bool RestoreSnapshot(const EntityWorldSnapshotAsset& snapshot, festd::vector<MaterializationToken>* operations);
    };
} // namespace FE::Framework
