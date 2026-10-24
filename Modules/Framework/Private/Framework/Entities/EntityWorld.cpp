#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityWorldInternal.h>

namespace FE::Framework
{
    namespace
    {
        std::atomic<uint32_t> GNextWorld{ 1 };
        EntityAssetServices GDefaultAssets;
    } // namespace


    EntityWorld::Impl::Impl(EntityWorld& owner, EntityAssetServices* assets)
        : m_owner(owner)
        , m_schedule(*this)
        , m_lifecycle(*this)
        , m_materializer(*this)
        , m_assets(assets ? assets : &GDefaultAssets)
    {
        FE_AssertDebug(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");

        const uint32_t id = GNextWorld.fetch_add(1);
        FE_Assert(id <= UINT16_MAX, "World ID space exhausted");
        m_id = static_cast<uint16_t>(id);
    }


    EntityWorld::Impl::~Impl()
    {
        while (!m_storage.m_registries.empty())
            RemoveRegistry(*m_storage.m_registries.back());

        m_schedule.Shutdown();
        for (uint32_t i = m_services.size(); i > 0; --i)
            m_services[i - 1]->Shutdown(m_owner);

        for (auto* commands : m_pendingCommands.m_lists)
            Memory::DefaultDelete(commands);

        for (auto* archetype : m_storage.m_archetypes)
            Memory::DefaultDelete(archetype);
    }


    EntityWorld::EntityWorld(EntityAssetServices* assets)
        : m_impl(Memory::DefaultNew<Impl>(*this, assets))
    {
    }


    EntityWorld::~EntityWorld()
    {
        Memory::DefaultDelete(m_impl);
    }


    bool EntityWorld::Impl::Fail(const festd::ascii_view message)
    {
        m_error = message;
        return false;
    }


    uint64_t EntityWorld::Impl::GetQuotaFrame() const
    {
        return m_explicitFrames ? m_frame : m_schedule.m_epoch;
    }


    Entity* EntityWorld::Find(const EntityID id) const
    {
        if (id.m_value == 0 || id.World() != m_impl->m_id || id.Slot() >= m_impl->m_storage.m_slots.size())
            return nullptr;

        const auto& slot = m_impl->m_storage.m_slots[id.Slot()];
        return slot.m_generation == id.Generation() ? slot.m_entity : nullptr;
    }


    Entity* EntityWorld::Find(const Uuid uuid, const bool activeOnly) const
    {
        const auto it = m_impl->m_storage.m_uuidLookup.find(uuid);
        if (it == m_impl->m_storage.m_uuidLookup.end())
            return nullptr;

        return !activeOnly || it->second->IsActive() ? it->second : nullptr;
    }


    void EntityWorld::TraverseRoots(void* context, bool (*visit)(void*, Entity&)) const
    {
        FE_AssertDebug(Threading::IsMainThread() && !m_impl->m_schedule.IsUpdating());
        for (Entity* root = m_impl->m_storage.m_firstRoot; root; root = root->m_nextRoot)
        {
            if (!visit(context, *root))
                break;
        }
    }


    festd::span<const EntityComponentInfo* const> EntityWorld::GetComponents(const Entity& entity) const
    {
        FE_AssertDebug(&entity.GetWorld() == this && !m_impl->m_schedule.IsUpdating());

        if (entity.m_chunk)
            return entity.m_chunk->m_archetype.m_columns;

        return {};
    }


    EntityRegistry& EntityWorld::Impl::CreateRegistry(Uuid key)
    {
        FE_AssertDebug(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_AssertDebug(!m_schedule.IsCollecting() && !m_schedule.IsExecuting());

        if (!key.IsValid())
            key = Uuid::Random();

        FE_Assert(!m_owner.FindRegistry(key), "Registry key is already owned");
        void* storage = Memory::DefaultAllocate(sizeof(EntityRegistry), alignof(EntityRegistry));
        auto* registry = ::new (storage) EntityRegistry(m_owner, *m_assets, m_storage.m_registryIds.Allocate(), key);
        m_storage.m_registries.push_back(registry);
        return *registry;
    }


    void EntityWorld::Impl::RemoveRegistry(EntityRegistry& registry)
    {
        FE_AssertDebug(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_AssertDebug(!m_schedule.IsExecuting() && !m_schedule.IsCollecting());

        const auto registryIt = festd::find(m_storage.m_registries, &registry);
        if (registryIt == m_storage.m_registries.end())
            return;

        registry.m_unloading = true;
        m_materializer.CancelRegistry(registry);

        for (auto& slot : m_storage.m_slots)
        {
            if (slot.m_entity && slot.m_entity->m_registry == &registry && !slot.m_entity->m_parent)
                DestroyEntity(*slot.m_entity);
        }

        // Cancel every pending list that names this owner before releasing its address.
        {
            std::lock_guard lock{ m_pendingCommands.m_lock };
            auto& lists = m_pendingCommands.m_lists;
            const auto references = [&registry](const EntityCommandList::Impl* list) {
                return eastl::any_of(list->m_commands.begin(), list->m_commands.end(), [&registry](const Command& command) {
                    return command.GetRegistry() == &registry;
                });
            };

            for (auto it = lists.begin(); it != lists.end();)
            {
                if (!references(*it))
                {
                    ++it;
                    continue;
                }

                Memory::DefaultDelete(*it);
                it = lists.erase(it);
            }
        }

        m_storage.m_registries.erase(registryIt);
        const uint32_t registryID = registry.GetID();
        Memory::DefaultDelete(&registry);
        m_storage.m_registryIds.Release(registryID);
    }


    bool EntityWorld::Impl::OwnsRegistry(const EntityRegistry& registry) const
    {
        const auto& registries = m_storage.m_registries;
        return eastl::find(registries.begin(), registries.end(), &registry) != registries.end();
    }


    EntityComponentRegistry& EntityWorld::Components()
    {
        return m_impl->m_components;
    }


    uint64_t EntityWorld::GetEpoch() const
    {
        return m_impl->m_schedule.m_epoch;
    }


    uint64_t EntityWorld::GetHierarchyRevision() const
    {
        return m_impl->m_storage.m_hierarchyRevision;
    }


    festd::ascii_view EntityWorld::GetLastError() const
    {
        return m_impl->m_error;
    }


    uint32_t EntityWorld::GetEntityCount() const
    {
        return static_cast<uint32_t>(m_impl->m_storage.m_uuidLookup.size());
    }


    uint32_t EntityWorld::GetChunkCount() const
    {
        return m_impl->m_storage.m_chunks.size();
    }


    Entity* EntityWorld::Impl::AllocateEntity(EntityRegistry& registry, const Env::Name name, const Uuid uuid)
    {
        if (m_storage.m_uuidLookup.contains(uuid))
            return nullptr;

        uint32_t index;
        if (m_storage.m_freeSlots.empty())
        {
            index = m_storage.m_slots.size();
            FE_Assert(index <= 0xffffff, "Entity slot space exhausted");
            m_storage.m_slots.push_back({});
        }
        else
        {
            index = m_storage.m_freeSlots.back();
            m_storage.m_freeSlots.pop_back();
        }

        auto& slot = m_storage.m_slots[index];
        const EntityID entityId = EntityID::Pack(m_id, index, slot.m_generation);
        Entity* entity = ::new (m_storage.m_entities.AllocateMemory()) Entity(registry, entityId, uuid, name);
        slot.m_entity = entity;
        m_storage.m_uuidLookup.emplace(uuid, entity);
        LinkRoot(*entity);
        return entity;
    }


    void EntityWorld::Impl::LinkRoot(Entity& entity)
    {
        entity.m_previousRoot = m_storage.m_lastRoot;
        entity.m_nextRoot = nullptr;
        if (m_storage.m_lastRoot)
            m_storage.m_lastRoot->m_nextRoot = &entity;
        else
            m_storage.m_firstRoot = &entity;

        m_storage.m_lastRoot = &entity;
    }


    void EntityWorld::Impl::UnlinkRoot(Entity& entity)
    {
        if (entity.m_previousRoot)
            entity.m_previousRoot->m_nextRoot = entity.m_nextRoot;
        else
            m_storage.m_firstRoot = entity.m_nextRoot;

        if (entity.m_nextRoot)
            entity.m_nextRoot->m_previousRoot = entity.m_previousRoot;
        else
            m_storage.m_lastRoot = entity.m_previousRoot;

        entity.m_previousRoot = nullptr;
        entity.m_nextRoot = nullptr;
    }


    void EntityWorld::Impl::Reparent(Entity& entity, Entity* parent)
    {
        if (entity.m_parent == parent)
            return;

        if (entity.m_parent)
            m_lifecycle.MarkUnready(*entity.m_parent);

        if (parent && !parent->m_active && entity.m_active)
            m_lifecycle.DeactivateSubtree(entity);

        if (entity.m_previousSibling)
            entity.m_previousSibling->m_nextSibling = entity.m_nextSibling;
        else if (entity.m_parent)
            entity.m_parent->m_firstChild = entity.m_nextSibling;

        if (entity.m_nextSibling)
            entity.m_nextSibling->m_previousSibling = entity.m_previousSibling;
        else if (entity.m_parent)
            entity.m_parent->m_lastChild = entity.m_previousSibling;

        if (!entity.m_parent)
            UnlinkRoot(entity);

        entity.m_parent = parent;
        if (!parent)
            LinkRoot(entity);

        entity.m_previousSibling = parent ? parent->m_lastChild.Get() : nullptr;
        entity.m_nextSibling = nullptr;

        if (parent)
        {
            if (parent->m_lastChild)
                parent->m_lastChild->m_nextSibling = &entity;
            else
                parent->m_firstChild = &entity;
            parent->m_lastChild = &entity;
        }

        m_lifecycle.MarkUnready(entity);

        FE_Assert(m_storage.m_hierarchyRevision != UINT64_MAX, "Hierarchy revision exhausted");
        ++m_storage.m_hierarchyRevision;
        MarkChanged(entity);
    }


    void EntityWorld::Impl::DestroyEntity(Entity& entity)
    {
        while (entity.m_firstChild)
            DestroyEntity(*entity.m_firstChild);
        m_lifecycle.DeactivateSubtree(entity, false);

        for (uint32_t i = entity.m_chunk->m_archetype.m_columns.size(); i > 0; --i)
            m_lifecycle.TeardownComponent(entity, i - 1, true);

        m_storage.FreeRow(*entity.m_chunk, entity.m_row);

        entity.m_chunk = nullptr;
        Reparent(entity, nullptr);
        UnlinkRoot(entity);
        m_storage.m_uuidLookup.erase(entity.m_uuid);

        auto& slot = m_storage.m_slots[entity.m_id.Slot()];
        slot.m_entity = nullptr;
        if (slot.m_generation < 0xffffff)
        {
            ++slot.m_generation;
            m_storage.m_freeSlots.push_back(entity.m_id.Slot());
        }

        entity.~Entity();
        m_storage.m_entities.GetAllocator()->deallocate(&entity, sizeof(Entity), alignof(Entity));
        FE_Assert(m_storage.m_hierarchyRevision != UINT64_MAX, "Hierarchy revision exhausted");
        ++m_storage.m_hierarchyRevision;
    }


    Archetype* EntityWorld::Impl::GetArchetype(const festd::span<const EntityComponentInfo* const> columns)
    {
        for (auto* archetype : m_storage.m_archetypes)
        {
            const bool sameLayout =
                std::equal(archetype->m_columns.begin(), archetype->m_columns.end(), columns.begin(), columns.end());
            if (sameLayout)
                return archetype;
        }

        auto* archetype = Memory::DefaultNew<Archetype>(columns);
        m_storage.m_archetypes.push_back(archetype);
        return archetype;
    }


    void EntityWorld::Impl::Migrate(Entity& entity, Archetype& archetype, const festd::span<void* const> values)
    {
        FE_PROFILER_ZONE();

        const auto& columns = archetype.m_columns;
        FE_AssertDebug(columns.size() == values.size());

        auto* source = entity.m_chunk;
        const uint32_t oldRow = entity.m_row;
        festd::vector<void*> actualValues(values.begin(), values.end());
        if (source && entity.m_active)
        {
            // Replacing an active component stages a candidate; the live value stays until the candidate is ready.
            for (uint32_t i = 0; i < columns.size(); ++i)
            {
                const uint32_t oldColumn = source->m_archetype.Find(columns[i]->m_type->m_id);
                if (!actualValues[i] || oldColumn == kInvalidIndex)
                    continue;

                if ((source->Stage(oldRow, oldColumn) & ComponentStage::kActive) == ComponentStage::kNone)
                    continue;

                m_lifecycle.StageReplacement(entity, *columns[i], actualValues[i]);
                actualValues[i] = nullptr;
            }
        }

        if (source && &source->m_archetype == &archetype)
        {
            // Same-layout edits never relocate retained values.
            for (uint32_t i = 0; i < columns.size(); ++i)
            {
                if (!actualValues[i])
                    continue;

                m_lifecycle.CancelReplacements(entity, columns[i]->m_type->m_id);
                m_lifecycle.TeardownComponent(entity, i, true);
                columns[i]->m_type->m_moveConstructor(source->Get(oldRow, i), actualValues[i]);
                source->Stage(oldRow, i) = ComponentStage::kNone;
            }

            m_lifecycle.MarkUnready(entity);
            MarkChanged(entity);
            return;
        }

        ArchetypeChunk* destination = &m_storage.GetAvailableChunk(archetype, *entity.m_registry);

        // Teardown removed/replaced components before moving their siblings.
        if (source)
        {
            for (uint32_t i = source->m_archetype.m_columns.size(); i > 0; --i)
            {
                const uint32_t oldColumn = i - 1;
                const auto* info = source->m_archetype.m_columns[oldColumn];

                const auto found = festd::find(columns, info);
                const bool retained = found != columns.end() && !actualValues[static_cast<uint32_t>(found - columns.begin())];
                if (!retained)
                {
                    m_lifecycle.CancelReplacements(entity, info->m_type->m_id);
                    m_lifecycle.TeardownComponent(entity, oldColumn, true);
                }
            }
        }

        const uint32_t row = destination->Allocate(entity);
        m_storage.RowAllocated(*destination);
        for (uint32_t i = 0; i < columns.size(); ++i)
        {
            const auto& type = *columns[i]->m_type;
            const uint32_t oldColumn = source ? source->m_archetype.Find(type.m_id) : kInvalidIndex;
            if (actualValues[i])
            {
                type.m_moveConstructor(destination->Get(row, i), actualValues[i]);
            }
            else if (oldColumn != kInvalidIndex)
            {
                type.m_moveConstructor(destination->Get(row, i), source->Get(oldRow, oldColumn));
                type.m_destructor(source->Get(oldRow, oldColumn));
                destination->Stage(row, i) = source->Stage(oldRow, oldColumn);
                destination->AddedVersion(row, i) = source->AddedVersion(oldRow, oldColumn);
            }
            else
            {
                FE_AssertDebug(type.m_defaultConstructor);
                type.m_defaultConstructor(destination->Get(row, i));
            }
        }

        FE_Assert(m_storage.m_structureRevision != UINT64_MAX, "Structure revision exhausted");
        ++m_storage.m_structureRevision;
        entity.m_chunk = destination;
        entity.m_row = row;
        m_lifecycle.MarkUnready(entity);
        MarkChanged(entity);

        if (source)
            m_storage.FreeRow(*source, oldRow);
    }


    void EntityWorld::SetReparentHandler(const ReparentHandler handler)
    {
        FE_AssertDebug(Threading::IsMainThread() && !m_impl->m_schedule.IsExecuting() && !m_impl->m_schedule.IsCollecting());
        FE_Assert(!handler || !m_impl->m_reparentHandler || handler == m_impl->m_reparentHandler);
        m_impl->m_reparentHandler = handler;
    }


    uint64_t EntityWorld::Impl::NextChangeVersion()
    {
        if (m_storage.m_changeVersion == UINT64_MAX)
        {
            FE_Assert(m_storage.m_changeEra != UINT64_MAX, "Change version era exhausted");
            ++m_storage.m_changeEra;
            m_storage.m_changeVersion = 0;

            for (auto* chunk : m_storage.m_chunks)
            {
                for (auto& version : chunk->m_versions)
                    version = 0;
            }
        }

        return ++m_storage.m_changeVersion;
    }


    void EntityWorld::Impl::MarkChanged(Entity& entity)
    {
        if (!entity.m_chunk)
            return;

        std::lock_guard guard(m_storage.m_versionLock);

        const uint64_t version = NextChangeVersion();
        for (auto& column : entity.m_chunk->m_versions)
            column = version;
    }


    void* EntityWorld::Impl::LookupComponent(const Entity& entity, const Rtti::TypeID type, const bool write) const
    {
        FE_AssertDebug(!m_schedule.IsCollecting(), "System collection cannot inspect live component data");

        const auto* callback = m_schedule.IsExecuting()
            ? static_cast<const CallbackContext*>(Threading::FiberRuntimeInfo::Get().m_executionContext)
            : nullptr;

        const auto* traversal = callback && callback->m_world == &m_owner ? callback->m_traversal : nullptr;
        if (traversal && m_schedule.IsValidationEnabled())
        {
            bool declared = false;
            for (const auto& access : traversal->m_accesses)
            {
                const Entity* source = access.m_parent ? callback->m_entity->GetParent() : callback->m_entity;
                if (!access.m_excluded && access.m_type == type && (!write || access.m_write) && source == &entity)
                    declared = true;
            }

            FE_Assert(declared, "Entity component lookup exceeds traversal access declarations");
        }

        if (!entity.m_chunk)
            return nullptr;

        const uint32_t column = entity.m_chunk->m_archetype.Find(type);
        if (column == kInvalidIndex)
            return nullptr;

        if (traversal && ((entity.m_chunk->Stage(entity.m_row, column) & ComponentStage::kActive) == ComponentStage::kNone))
            return nullptr;

        return entity.m_chunk->Get(entity.m_row, column);
    }


    void EntityWorld::RequireAsset(Entity& entity, const IO::AssetID id, const Rtti::TypeID type)
    {
        m_impl->m_lifecycle.RequireAsset(entity, id, type);
    }


    void EntityWorld::AddSystem(WorldSystem& system)
    {
        m_impl->m_schedule.AddSystem(system);
    }


    void EntityWorld::RemoveSystem(WorldSystem& system)
    {
        m_impl->m_schedule.RemoveSystem(system);
    }


    void EntityWorld::AddService(WorldService& service)
    {
        FE_AssertDebug(Threading::IsMainThread() && !m_impl->m_schedule.IsUpdating());
        FE_Assert(festd::find(m_impl->m_services, &service) == m_impl->m_services.end());
        m_impl->m_services.push_back(&service);
        service.Init(*this);
    }


    void EntityWorld::RemoveService(WorldService& service)
    {
        FE_AssertDebug(Threading::IsMainThread() && !m_impl->m_schedule.IsUpdating() && GetEntityCount() == 0);
        const auto found = festd::find(m_impl->m_services, &service);
        if (found == m_impl->m_services.end())
            return;

        service.Shutdown(*this);
        m_impl->m_services.erase(found);
    }


    void* EntityWorld::FindService(const Rtti::TypeID type) const
    {
        for (WorldService* service : m_impl->m_services)
        {
            if (void* result = service->RTTI_TryCast(type))
                return result;
        }

        return nullptr;
    }


    void EntityWorld::BeginFrame()
    {
        FE_AssertDebug(Threading::IsMainThread() && !m_impl->m_schedule.IsUpdating());
        m_impl->m_frame = Math::Max(m_impl->m_frame, m_impl->m_schedule.m_epoch) + 1;
        m_impl->m_explicitFrames = true;
    }


    void EntityWorld::BeginUpdate()
    {
        m_impl->m_schedule.BeginUpdate();
    }


    void EntityWorld::BeginFixedUpdate(const float deltaSeconds)
    {
        FE_AssertDebug(deltaSeconds > 0);
        m_impl->m_schedule.BeginUpdate(deltaSeconds);
    }


    bool EntityWorld::RunFixedUpdates(const double deltaSeconds, const float fixedDeltaSeconds, const uint32_t maxSteps,
                                      void (*schedule)(void*, EntityWorld&), void* callable)
    {
        FE_AssertDebug(Threading::IsMainThread() && !m_impl->m_schedule.IsUpdating());

        const bool validDelta = std::isfinite(deltaSeconds) && deltaSeconds >= 0;
        const bool validStep = std::isfinite(fixedDeltaSeconds) && fixedDeltaSeconds > 0;
        if (!validDelta || !validStep || maxSteps == 0)
            return false;

        const double stepSeconds = fixedDeltaSeconds;
        m_impl->m_fixedAccumulator += Math::Min(deltaSeconds, stepSeconds * maxSteps);
        uint32_t steps = 0;
        while (m_impl->m_fixedAccumulator >= stepSeconds && steps < maxSteps)
        {
            BeginFixedUpdate(fixedDeltaSeconds);
            schedule(callable, *this);
            const bool executed = ExecuteSchedule();
            const bool ended = EndUpdate();
            if (!executed || !ended)
                return false;

            m_impl->m_fixedAccumulator -= stepSeconds;
            ++steps;
        }

        if (m_impl->m_fixedAccumulator >= stepSeconds)
            m_impl->m_fixedAccumulator = std::fmod(m_impl->m_fixedAccumulator, stepSeconds);

        return true;
    }


    bool EntityWorld::EndUpdate()
    {
        return m_impl->m_schedule.EndUpdate();
    }


    void* EntityWorld::AllocateTraversal(const size_t size, const size_t alignment)
    {
        return m_impl->m_schedule.AllocateTraversal(size, alignment);
    }


    Rc<WaitGroup> EntityWorld::RecordTraversal(EntityUpdateContext& context, const TraversalDesc& desc)
    {
        return m_impl->m_schedule.RecordTraversal(context, desc);
    }


    Rc<WaitGroup> EntityWorld::RecordStage(const StageDesc& desc)
    {
        return m_impl->m_schedule.RecordStage(desc);
    }


    bool EntityWorld::AddPrerequisite(WaitGroup& completion, const Rc<WaitGroup>& prerequisite)
    {
        return m_impl->m_schedule.AddPrerequisite(completion, prerequisite);
    }


    bool EntityWorld::SchedulePhase(const Phase phase, const festd::span<const Rc<WaitGroup>> prerequisites)
    {
        return m_impl->m_schedule.SchedulePhase(phase, prerequisites);
    }


    bool EntityWorld::ValidateSchedule()
    {
        return m_impl->m_schedule.ValidateSchedule();
    }


    bool EntityWorld::ExecuteSchedule()
    {
        return m_impl->m_schedule.ExecuteSchedule();
    }


    ScheduleDiagnostics EntityWorld::GetScheduleDiagnostics() const
    {
        return m_impl->m_schedule.GetDiagnostics();
    }


    festd::span<const ScheduleConflict> EntityWorld::GetScheduleConflicts() const
    {
        return m_impl->m_schedule.GetConflicts();
    }


    EntityRegistry& EntityWorld::CreateRegistry(const Uuid key)
    {
        return m_impl->CreateRegistry(key);
    }


    void EntityWorld::RemoveRegistry(EntityRegistry& registry)
    {
        m_impl->RemoveRegistry(registry);
    }


    EntityRegistry* EntityWorld::FindRegistry(const Uuid key) const
    {
        for (EntityRegistry* registry : m_impl->m_storage.m_registries)
        {
            if (registry->GetKey() == key)
                return registry;
        }

        return nullptr;
    }


    void EntityWorld::Clear()
    {
        while (!m_impl->m_storage.m_registries.empty())
            RemoveRegistry(*m_impl->m_storage.m_registries.back());
    }


    void* EntityWorld::LookupComponent(const Entity& entity, const Rtti::TypeID type, const bool write) const
    {
        return m_impl->LookupComponent(entity, type, write);
    }


    void EntityWorld::Submit(EntityCommandList&& commands)
    {
        m_impl->Submit(std::move(commands));
    }


    MaterializationToken EntityWorld::SpawnCollection(EntityRegistry& registry, const EntityCollection& collection)
    {
        return m_impl->m_materializer.Spawn(registry, collection);
    }


    MaterializationToken EntityWorld::SpawnCollection(EntityRegistry& registry, const IO::AssetID collection)
    {
        return m_impl->m_materializer.Spawn(registry, collection);
    }


    MaterializationToken EntityWorld::LoadEntities(EntityRegistry& registry, const EntityCollection& entities)
    {
        return m_impl->m_materializer.LoadEntities(registry, entities);
    }


    MaterializationToken EntityWorld::LoadPlacement(EntityRegistry& registry, const IO::AssetID placement)
    {
        return m_impl->m_materializer.LoadPlacement(registry, placement);
    }


    MaterializationToken EntityWorld::LoadPlacement(EntityRegistry& registry, const IO::AssetID placement,
                                                    const EntityCollectionInstanceAsset& definition,
                                                    const EntityCollection& collection)
    {
        return m_impl->m_materializer.LoadPlacement(registry, placement, definition, collection);
    }


    void EntityWorld::CancelMaterialization(const MaterializationToken token)
    {
        FE_AssertDebug(!m_impl->m_schedule.IsCollecting() && !m_impl->m_schedule.IsExecuting());
        m_impl->m_materializer.Cancel(token);
    }


    MaterializationStatus EntityWorld::GetMaterializationStatus(const MaterializationToken token) const
    {
        return m_impl->m_materializer.GetStatus(token);
    }


    festd::span<const EntityUuidBinding> EntityWorld::GetMaterializationBindings(const MaterializationToken token) const
    {
        return m_impl->m_materializer.GetBindings(token);
    }


    bool EntityWorld::CommitBootstrap()
    {
        FE_Assert(!m_impl->m_started, "Bootstrap commits end with the first update");
        if (m_impl->m_started)
            return false;

        return m_impl->CommitImpl(true);
    }


    bool EntityWorld::Commit()
    {
        return m_impl->CommitImpl(false);
    }


    bool EntityWorld::LoadDefinition(const EntityWorldAsset& definition, festd::vector<MaterializationToken>* operations)
    {
        return m_impl->LoadDefinition(definition, operations);
    }


    bool EntityWorld::CaptureSnapshot(EntityWorldSnapshotAsset& snapshot) const
    {
        return m_impl->CaptureSnapshot(snapshot);
    }


    bool EntityWorld::RestoreSnapshot(const EntityWorldSnapshotAsset& snapshot, festd::vector<MaterializationToken>* operations)
    {
        return m_impl->RestoreSnapshot(snapshot, operations);
    }
} // namespace FE::Framework
