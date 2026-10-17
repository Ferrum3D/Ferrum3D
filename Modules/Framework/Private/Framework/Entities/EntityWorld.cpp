#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Serialization/DependencySerialization.h>
#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntitySerialization.h>
#include <Framework/Entities/EntityWorldAsset.h>
#include <Framework/Entities/EntityWorldInternal.h>
#include <algorithm>

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
    {
        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");

        const uint32_t token = GNextWorld.fetch_add(1);
        FE_Assert(token <= 0xffff, "World incarnation space exhausted");
        m_token = static_cast<uint16_t>(token);
        m_assets = assets ? assets : &GDefaultAssets;
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

        for (auto* operation : m_materializations)
            Memory::DefaultDelete(operation);
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


    Entity* EntityWorld::Find(const EntityID id) const
    {
        if (id.m_value == 0 || id.World() != m_impl->m_token || id.Slot() >= m_impl->m_storage.m_slots.size())
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


    EntityRegistry& EntityWorld::Impl::CreateRegistry(Uuid key)
    {
        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_Assert(!m_schedule.IsCollecting() && !m_schedule.IsExecuting());

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
        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_Assert(!m_schedule.IsExecuting() && !m_schedule.IsCollecting());

        const auto registryIt = festd::find(m_storage.m_registries, &registry);
        if (registryIt == m_storage.m_registries.end())
            return;

        registry.m_unloading = true;
        for (uint32_t index = 0; index < m_materializations.size(); ++index)
        {
            const auto& operation = *m_materializations[index];
            if (operation.m_registry == &registry && operation.m_state <= MaterializationState::kReady)
                CancelMaterialization({ m_token, index });
        }

        for (auto& slot : m_storage.m_slots)
        {
            if (slot.m_entity && slot.m_entity->m_registry == &registry && !slot.m_entity->m_parent)
                DestroyEntity(*slot.m_entity);
        }

        // Cancel every pending list that names this owner before releasing its address.
        {
            std::lock_guard lock{ m_pendingCommands.m_lock };
            for (auto it = m_pendingCommands.m_lists.begin(); it != m_pendingCommands.m_lists.end();)
            {
                bool references = false;
                for (const auto& command : (*it)->m_commands)
                {
                    if (command.GetRegistry() == &registry)
                        references = true;
                }

                if (references)
                {
                    Memory::DefaultDelete(*it);
                    it = m_pendingCommands.m_lists.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        m_storage.m_registries.erase(registryIt);
        const uint32_t registryID = registry.GetID();
        Memory::DefaultDelete(&registry);
        m_storage.m_registryIds.Release(registryID);
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
        const EntityID entityId = EntityID::Pack(m_token, index, slot.m_generation);
        Entity* entity = ::new (m_storage.m_entities.AllocateMemory()) Entity(registry, entityId, uuid, name);
        slot.m_entity = entity;
        m_storage.m_uuidLookup.emplace(uuid, entity);
        return entity;
    }


    void EntityWorld::Impl::Reparent(Entity& entity, Entity* parent)
    {
        if (entity.m_parent == parent)
            return;

        if (entity.m_parent)
            MarkUnready(*entity.m_parent);

        if (parent && !parent->m_active && entity.m_active)
            DeactivateSubtree(entity);

        if (entity.m_previousSibling)
            entity.m_previousSibling->m_nextSibling = entity.m_nextSibling;
        else if (entity.m_parent)
            entity.m_parent->m_firstChild = entity.m_nextSibling;

        if (entity.m_nextSibling)
            entity.m_nextSibling->m_previousSibling = entity.m_previousSibling;
        else if (entity.m_parent)
            entity.m_parent->m_lastChild = entity.m_previousSibling;

        entity.m_parent = parent;
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

        MarkUnready(entity);

        FE_Assert(m_storage.m_hierarchyRevision != UINT64_MAX, "Hierarchy revision exhausted");
        ++m_storage.m_hierarchyRevision;
        MarkChanged(entity);
    }


    void EntityWorld::Impl::DestroyEntity(Entity& entity)
    {
        while (entity.m_firstChild)
            DestroyEntity(*entity.m_firstChild);
        DeactivateSubtree(entity, false);

        const auto& order = entity.m_chunk->m_archetype.m_lifecycleOrder;
        for (uint32_t i = order.size(); i > 0; --i)
            TeardownComponent(entity, order[i - 1], true);

        auto* chunk = entity.m_chunk;
        m_storage.FreeRow(*chunk, entity.m_row);

        entity.m_chunk = nullptr;
        Reparent(entity, nullptr);
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
        if (archetype->m_lifecycleOrder.size() != columns.size())
        {
            Memory::DefaultDelete(archetype);
            return nullptr;
        }

        m_storage.m_archetypes.push_back(archetype);
        return archetype;
    }


    void EntityWorld::Impl::Migrate(Entity& entity, Archetype& layout, const festd::span<void* const> values)
    {
        FE_PROFILER_ZONE();

        Archetype* archetype = &layout;
        const auto& columns = layout.m_columns;
        FE_Assert(columns.size() == values.size());

        auto* source = entity.m_chunk;
        const uint32_t oldRow = entity.m_row;
        festd::vector<void*> actualValues(values.begin(), values.end());
        if (source && entity.m_active)
        {
            for (uint32_t i = 0; i < columns.size(); ++i)
            {
                const auto& type = *columns[i]->m_type;
                const uint32_t oldColumn = source->m_archetype.Find(type.m_id);
                if (!actualValues[i] || oldColumn == kInvalidIndex)
                    continue;

                if ((source->Stage(oldRow, oldColumn) & ComponentStage::kActive) == ComponentStage::kNone)
                    continue;

                CancelReplacements(entity, type.m_id);
                void* data = Memory::DefaultAllocate(type.m_size, type.m_alignment);
                type.m_moveConstructor(data, actualValues[i]);
                GetResources(entity).m_replacements.push_back({ columns[i], data, m_loading.m_nextTransition++ });
                if (festd::find(m_lifecycleWork.m_replacements, entity.m_id) == m_lifecycleWork.m_replacements.end())
                    m_lifecycleWork.m_replacements.push_back(entity.m_id);

                actualValues[i] = nullptr;
            }
        }

        if (source && &source->m_archetype == archetype)
        {
            // Same-layout edits never relocate retained values; active candidates remain staged until ready.
            for (uint32_t i = 0; i < columns.size(); ++i)
            {
                if (!actualValues[i])
                    continue;

                CancelReplacements(entity, columns[i]->m_type->m_id);
                TeardownComponent(entity, i, true);
                columns[i]->m_type->m_moveConstructor(source->Get(oldRow, i), actualValues[i]);
                source->Stage(oldRow, i) = ComponentStage::kNone;
            }

            MarkUnready(entity);
            MarkChanged(entity);
            return;
        }

        ArchetypeChunk* destination = &m_storage.GetAvailableChunk(*archetype, *entity.m_registry);

        // Teardown removed/replaced components before moving their siblings.
        if (source)
        {
            const auto& order = source->m_archetype.m_lifecycleOrder;
            for (uint32_t i = order.size(); i > 0; --i)
            {
                const uint32_t oldColumn = order[i - 1];
                const auto* info = source->m_archetype.m_columns[oldColumn];

                const auto found = festd::find(columns, info);
                const bool retained = found != columns.end() && !actualValues[static_cast<uint32_t>(found - columns.begin())];
                if (!retained)
                {
                    CancelReplacements(entity, info->m_type->m_id);
                    TeardownComponent(entity, oldColumn, true);
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
            }
            else
            {
                FE_Assert(type.m_defaultConstructor);
                type.m_defaultConstructor(destination->Get(row, i));
            }
        }

        FE_Assert(m_storage.m_structureRevision != UINT64_MAX, "Structure revision exhausted");
        ++m_storage.m_structureRevision;
        entity.m_chunk = destination;
        entity.m_row = row;
        MarkUnready(entity);
        MarkChanged(entity);

        if (source)
            m_storage.FreeRow(*source, oldRow);
    }


    void EntityWorld::SetReparentHandler(const ReparentHandler handler)
    {
        FE_Assert(Threading::IsMainThread() && !m_impl->m_schedule.IsExecuting() && !m_impl->m_schedule.IsCollecting());
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
        FE_Assert(!m_schedule.IsCollecting(), "System collection cannot inspect live component data");

        const auto* callback = m_schedule.IsExecuting()
            ? static_cast<const CallbackContext*>(Threading::FiberRuntimeInfo::Get().m_executionContext)
            : nullptr;

        const auto* traversal = callback && callback->m_world == &m_owner ? callback->m_traversal : nullptr;
        if (traversal)
        {
            bool declared = false;
            for (const auto& access : traversal->m_accesses)
            {
                const Entity* source = access.m_parent ? callback->m_entity->GetParent() : callback->m_entity;
                if (!access.m_excluded && access.m_type == type && (!write || access.m_write) && source == &entity)
                    declared = true;
            }

            FE_AssertDebug(declared, "Entity component lookup exceeds traversal access declarations");
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
        m_impl->RequireAsset(entity, id, type);
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
        FE_Assert(Threading::IsMainThread() && !m_impl->m_schedule.IsUpdating());
        FE_Assert(festd::find(m_impl->m_services, &service) == m_impl->m_services.end());
        m_impl->m_services.push_back(&service);
        service.Init(*this);
    }


    void EntityWorld::RemoveService(WorldService& service)
    {
        FE_Assert(Threading::IsMainThread() && !m_impl->m_schedule.IsUpdating() && GetEntityCount() == 0);
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
        FE_Assert(Threading::IsMainThread() && !m_impl->m_schedule.IsUpdating());
        m_impl->m_materializationFrame = Math::Max(m_impl->m_materializationFrame, m_impl->m_schedule.m_epoch) + 1;
        m_impl->m_explicitMaterializationFrames = true;
    }


    void EntityWorld::BeginUpdate()
    {
        m_impl->m_schedule.BeginUpdate();
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
        if (0)
            return m_impl->m_schedule.ValidateSchedule();
        else
            return true;
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
        return m_impl->SpawnCollection(registry, collection);
    }


    MaterializationToken EntityWorld::SpawnCollection(EntityRegistry& registry, const IO::AssetID collection)
    {
        return m_impl->SpawnCollection(registry, collection);
    }


    MaterializationToken EntityWorld::LoadPlacement(EntityRegistry& registry, const IO::AssetID placement)
    {
        return m_impl->LoadPlacement(registry, placement);
    }


    MaterializationToken EntityWorld::LoadPlacement(EntityRegistry& registry, const IO::AssetID placement,
                                                    const EntityCollectionInstanceAsset& definition,
                                                    const EntityCollection& collection)
    {
        return m_impl->LoadPlacement(registry, placement, definition, collection);
    }


    void EntityWorld::CancelMaterialization(const MaterializationToken token)
    {
        m_impl->CancelMaterialization(token);
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


    MaterializationStatus EntityWorld::GetMaterializationStatus(const MaterializationToken token) const
    {
        if (token.m_world != m_impl->m_token || token.m_index >= m_impl->m_materializations.size())
            return {};

        const auto& operation = *m_impl->m_materializations[token.m_index];
        return { operation.m_state, operation.m_root, festd::string_view(operation.m_error.data(), operation.m_error.size()) };
    }


    festd::span<const EntityUuidBinding> EntityWorld::GetMaterializationBindings(const MaterializationToken token) const
    {
        if (token.m_world != m_impl->m_token || token.m_index >= m_impl->m_materializations.size())
            return {};

        return m_impl->m_materializations[token.m_index]->m_bindings;
    }


    MaterializationToken EntityWorld::Impl::SpawnCollection(EntityRegistry& registry, const EntityCollection& collection)
    {
        FE_Assert(Threading::IsMainThread());
        FE_Assert(&registry.GetWorld() == &m_owner);
        FE_Assert(m_materializations.size() < UINT32_MAX, "Materialization token space exhausted");

        EntityMaterialization operation;
        operation.m_registry = &registry;
        operation.m_registryId = registry.GetID();
        operation.m_eligibleEpoch = m_schedule.m_epoch + 1;

        const bool registryOwned = festd::find(m_storage.m_registries, &registry) != m_storage.m_registries.end();
        if (!registryOwned || registry.m_unloading)
        {
            operation.m_state = MaterializationState::kCanceled;
        }

        if (collection.Validate())
        {
            operation.m_collection = collection;
        }
        else
        {
            operation.m_state = MaterializationState::kFailed;
            operation.m_error = "Invalid entity asset";
        }

        operation.m_hasDefinition = true;

        const MaterializationToken token{ m_token, (m_materializations.size()) };
        m_materializations.push_back(Memory::DefaultNew<EntityMaterialization>(std::move(operation)));
        if (m_materializations.back()->m_state == MaterializationState::kPending)
            m_activeMaterializations.push_back(token.m_index);

        return token;
    }


    MaterializationToken EntityWorld::Impl::SpawnCollection(EntityRegistry& registry, const IO::AssetID collection)
    {
        auto token = SpawnCollection(registry, EntityCollection{});
        auto& operation = *m_materializations[token.m_index];
        operation.m_hasDefinition = false;
        operation.m_asset = collection;
        if (operation.m_state == MaterializationState::kPending)
            operation.m_request = IO::AssetManager::LoadAsset(IO::Link<EntityCollection>(collection));

        return token;
    }


    MaterializationToken EntityWorld::Impl::LoadPlacement(EntityRegistry& registry, const IO::AssetID placement)
    {
        FE_Assert(Threading::IsMainThread());

        const auto existing = m_placements.find(placement);
        if (existing != m_placements.end())
            return { m_token, existing->second };

        auto token = SpawnCollection(registry, EntityCollection{});
        auto& operation = *m_materializations[token.m_index];
        operation.m_placement = true;
        m_placements[placement] = token.m_index;
        operation.m_hasDefinition = false;
        operation.m_asset = placement;
        if (operation.m_state == MaterializationState::kPending)
            operation.m_request = IO::AssetManager::LoadAsset(IO::Link<EntityCollectionInstanceAsset>(placement));

        if (operation.m_state > MaterializationState::kReady)
            m_placements.erase(placement);

        return token;
    }


    MaterializationToken EntityWorld::Impl::LoadPlacement(EntityRegistry& registry, const IO::AssetID placement,
                                                          const EntityCollectionInstanceAsset& definition,
                                                          const EntityCollection& collection)
    {
        FE_Assert(Threading::IsMainThread());

        const auto existing = m_placements.find(placement);
        if (existing != m_placements.end())
            return { m_token, existing->second };

        auto token = SpawnCollection(registry, collection);
        auto& operation = *m_materializations[token.m_index];
        operation.m_placement = true;
        m_placements[placement] = token.m_index;
        operation.m_asset = placement;

        if (definition.Validate())
        {
            operation.m_definition = definition;
        }
        else
        {
            operation.m_state = MaterializationState::kFailed;
            operation.m_error = "Invalid entity asset";
        }

        if (operation.m_state > MaterializationState::kReady)
            m_placements.erase(placement);

        return token;
    }


    void EntityWorld::Impl::CancelMaterialization(const MaterializationToken token)
    {
        FE_Assert(Threading::IsMainThread());
        FE_Assert(!m_schedule.IsCollecting() && !m_schedule.IsExecuting());

        if (token.m_world != m_token || token.m_index >= m_materializations.size())
            return;

        auto& operation = *m_materializations[token.m_index];
        if (operation.m_state == MaterializationState::kCanceled)
            return;

        operation.m_state = MaterializationState::kCanceled;
        if (operation.m_placement)
        {
            const auto placement = m_placements.find(operation.m_asset);
            if (placement != m_placements.end() && placement->second == token.m_index)
                m_placements.erase(placement);
        }

        operation.m_request.Reset();
        for (const auto id : operation.m_entities)
        {
            if (auto* entity = m_owner.Find(id))
                DestroyEntity(*entity);
        }

        operation.m_entities.clear();
        operation.m_schemas.clear();
        operation.m_schemas.shrink_to_fit();
        operation.m_collection = {};
        operation.m_definition = {};
        operation.m_root = {};
    }


    bool EntityWorld::Impl::Materialize(const uint32_t operationIndex, uint32_t* budget)
    {
        FE_PROFILER_ZONE();

        auto& operation = *m_materializations[operationIndex];
        const auto& collection = operation.m_collection;

        // Validate the complete authored definition before allocating any runtime membership.

        auto reject = [&](const festd::ascii_view error = "Invalid entity asset") {
            operation.m_error = error;
            operation.m_state = MaterializationState::kFailed;
            return false;
        };

        for (const EntityID id : operation.m_entities)
        {
            if (!m_owner.Find(id))
                return reject("Materialization member was destroyed");
        }

        if (!operation.m_validated)
        {
            if (operation.m_placement)
            {
                if (!operation.m_asset.IsValid() || !operation.m_definition.Validate(collection))
                    return reject();
            }
            else if (!collection.Validate())
            {
                return reject();
            }

            operation.m_bindings = operation.m_placement ? operation.m_definition.m_bindings : festd::vector<EntityUuidBinding>{};
            if (!operation.m_placement)
            {
                for (const auto& entity : collection.m_entities)
                    operation.m_bindings.push_back({ entity.m_uuid, operation.m_concrete ? entity.m_uuid : Uuid::Random() });
            }

            operation.m_rootUuid = operation.m_placement ? operation.m_definition.m_rootUuid : Uuid::Random();
            const Uuid rootUuid = operation.m_rootUuid;
            if (!operation.m_concrete && m_owner.Find(rootUuid, false))
                return reject("Placement root UUID is already owned");

            for (const auto& binding : operation.m_bindings)
            {
                if (m_owner.Find(binding.m_entityUuid, false))
                    return reject("Concrete entity UUID is already owned");
            }

            const uint32_t recordCount = collection.m_entities.size() + (operation.m_concrete ? 0 : 1);
            for (uint32_t row = 0; row < recordCount; ++row)
            {
                const uint32_t sourceIndex = row - (operation.m_concrete ? 0 : 1);
                const EntityRecord emptyRoot;
                const EntityRecord* source = row == 0 && !operation.m_concrete
                    ? (operation.m_placement ? &operation.m_definition.m_root.m_entities.front() : &emptyRoot)
                    : &collection.m_entities[sourceIndex];
                festd::vector<const EntityComponentInfo*> columns;
                for (const auto& component : source->m_components)
                {
                    const auto* info = m_components.Find(component.GetType()->m_id);
                    if (!info || info->m_policy.m_transient || !info->m_type->m_deserialize
                        || !info->m_type->m_defaultConstructor)
                        return reject();

                    columns.push_back(info);
                }

                for (uint32_t column = 0; column < columns.size(); ++column)
                {
                    for (const auto companion : columns[column]->m_runtimeCompanions)
                    {
                        const auto* info = m_components.Find(companion);
                        FE_Assert(info && info->m_policy.m_transient && info->m_type->m_defaultConstructor);

                        if (festd::find(columns, info) == columns.end())
                            columns.push_back(info);
                    }
                }

                festd::sort(columns.begin(), columns.end(), [](const EntityComponentInfo* a, const EntityComponentInfo* b) {
                    return a->m_type->m_id < b->m_type->m_id;
                });

                Archetype* archetype = GetArchetype(columns);
                if (!archetype)
                    return reject();

                operation.m_schemas.push_back(archetype);
            }

            operation.m_validated = true;
        }

        const uint32_t recordCount = collection.m_entities.size() + (operation.m_concrete ? 0 : 1);
        EntityRecord emptyRoot;
        emptyRoot.m_uuid = operation.m_rootUuid;
        const auto recordAt = [&](uint32_t row) -> const EntityRecord& {
            if (!operation.m_concrete)
            {
                if (row == 0)
                    return operation.m_placement ? operation.m_definition.m_root.m_entities.front() : emptyRoot;

                --row;
            }

            return collection.m_entities[row];
        };

        auto remap = [](void* data, const Uuid uuid) {
            auto& bindings = *static_cast<festd::vector<EntityUuidBinding>*>(data);
            for (const auto& binding : bindings)
            {
                if (binding.m_sourceUuid == uuid)
                    return binding.m_entityUuid;
            }

            return uuid;
        };

        bool success = false;
        auto rollback = festd::defer([&] {
            if (success)
                return;

            for (const auto id : operation.m_entities)
            {
                if (auto* entity = m_owner.Find(id))
                    DestroyEntity(*entity);
            }

            operation.m_entities.clear();
            operation.m_root = {};
        });

        uint32_t unlimited = UINT32_MAX;
        uint32_t& remaining = budget ? *budget : unlimited;
        for (uint32_t row = operation.m_entities.size(); row < recordCount && remaining; ++row)
        {
            --remaining;
            const auto& record = recordAt(row);
            Entity* entity = AllocateEntity(*operation.m_registry,
                                            Env::Name(record.m_name),
                                            row == 0 && !operation.m_concrete ? operation.m_rootUuid
                                                                              : remap(&operation.m_bindings, record.m_uuid));
            if (!entity)
                return reject("Concrete entity UUID is already owned");
            entity->m_wantsActive = false;
            entity->m_residencyScope = record.m_residency;

            const auto& columns = operation.m_schemas[row]->m_columns;
            festd::vector<void*> values(columns.size(), nullptr);
            Migrate(*entity, *operation.m_schemas[row], values);
            operation.m_entities.push_back(entity->m_id);

            // Copy only authored fields; deserialization hooks remap internal references to this membership.
            for (const auto& component : record.m_components)
            {
                const auto* info = m_components.Find(component.GetType()->m_id);
                void* destination = entity->FindComponent(info->m_type->m_id);
                using Dependency = std::pair<IO::AssetID, Rtti::TypeID>;
                festd::inline_vector<Dependency> dependencies;
                if (!CopyComponent(*info->m_type,
                                   component.GetValue(),
                                   destination,
                                   operation.m_bindings,
                                   &dependencies,
                                   [](void* user, Uuid asset, Rtti::TypeID type, const uint32_t kind) {
                                       if (kind == festd::to_underlying(IO::DependencyKind::kHard))
                                           static_cast<festd::inline_vector<Dependency>*>(user)->emplace_back(asset, type);
                                   }))
                {
                    return reject();
                }

                // Install ownership only after the authored copy succeeds; LoadValue can skip a second serialization pass.
                m_loading.m_component = info->m_type->m_id;
                for (const auto& [asset, type] : dependencies)
                    RequireAsset(*entity, asset, type);

                m_loading.m_component = Rtti::TypeID::kNull;
                const uint32_t column = entity->m_chunk->m_archetype.Find(info->m_type->m_id);
                entity->m_chunk->Stage(entity->m_row, column) |= ComponentStage::kDiscovered;
            }
        }

        if (operation.m_entities.size() < recordCount)
        {
            success = true;
            return true;
        }

        if (!operation.m_hierarchyInstalled)
        {
            operation.m_root = operation.m_entities.empty() ? EntityID{} : operation.m_entities.front();
            for (uint32_t row = operation.m_concrete ? 0 : 1; row < recordCount; ++row)
            {
                const Uuid parent = recordAt(row).m_parentUuid;
                Entity* parentEntity = parent.IsValid() ? m_owner.Find(remap(&operation.m_bindings, parent), false)
                                                        : (operation.m_concrete ? nullptr : m_owner.Find(operation.m_root));
                Reparent(*m_owner.Find(operation.m_entities[row]), parentEntity);
            }

            operation.m_hierarchyInstalled = true;
        }

        for (; operation.m_loadCount < operation.m_entities.size() && remaining; ++operation.m_loadCount)
        {
            --remaining;
            Entity& entity = *m_owner.Find(operation.m_entities[operation.m_loadCount]);
            for (uint32_t column = 0; column < entity.m_chunk->m_archetype.m_columns.size(); ++column)
            {
                const auto& info = *entity.m_chunk->m_archetype.m_columns[column];
                auto& stage = entity.m_chunk->Stage(entity.m_row, column);
                if (LoadValue(entity, info, entity.m_chunk->Get(entity.m_row, column), stage) == LifecycleResult::kFailed)
                    return reject();
            }
        }

        if (operation.m_loadCount < operation.m_entities.size())
        {
            success = true;
            return true;
        }

        for (uint32_t row = 0; row < operation.m_entities.size(); ++row)
        {
            Entity& entity = *m_owner.Find(operation.m_entities[row]);
            entity.m_wantsActive = recordAt(row).m_active;
            MarkUnready(entity);
        }

        operation.m_installed = true;
        operation.m_schemas.clear();
        operation.m_schemas.shrink_to_fit();
        success = true;
        return true;
    }


    void EntityWorld::Impl::AdvanceMaterializations(const bool bootstrap)
    {
        FE_PROFILER_ZONE();

        const uint64_t frame = m_explicitMaterializationFrames ? m_materializationFrame : m_schedule.m_epoch;
        if (m_materializationBudgetEpoch != frame)
        {
            m_materializationBudgetEpoch = frame;
            m_materializationsRemaining = 32;
        }

        const uint32_t operationCount = m_activeMaterializations.size();
        for (uint32_t active = 0; active < operationCount; ++active)
        {
            const uint32_t index = m_activeMaterializations[active];
            auto& operation = *m_materializations[index];
            if (operation.m_state == MaterializationState::kReady && !operation.m_concrete && !m_owner.Find(operation.m_root))
                m_owner.CancelMaterialization({ m_token, index });

            if (operation.m_state != MaterializationState::kPending
                || (!bootstrap && operation.m_eligibleEpoch > m_schedule.m_epoch))
            {
                continue;
            }

            const bool registryPresent =
                festd::find(m_storage.m_registries, operation.m_registry) != m_storage.m_registries.end();
            if (!registryPresent || operation.m_registry->GetID() != operation.m_registryId)
            {
                m_owner.CancelMaterialization({ m_token, index });
                continue;
            }

            auto fail = [&] {
                // Copy Core's owned diagnostic before cancellation releases the request.
                const auto error = operation.m_request.GetError();
                operation.m_error = error.empty() ? festd::string_view("Entity asset unavailable") : error;
                m_owner.CancelMaterialization({ m_token, index });
                operation.m_state = MaterializationState::kFailed;
            };

            if (operation.m_entities.empty() && !operation.m_hasDefinition)
            {
                if (!operation.m_request.IsValid())
                {
                    fail();
                    continue;
                }

                if (!operation.m_request.IsCompleted())
                    continue;

                if (operation.m_request.GetResult() != IO::AssetLoadResult::kSucceeded)
                {
                    fail();
                    continue;
                }

                if (operation.m_placement)
                {
                    auto placement = IO::AssetHandle<EntityCollectionInstanceAsset>(operation.m_request.GetAssetSlot()).Read();
                    if (!placement || !placement->Validate())
                    {
                        fail();
                        continue;
                    }

                    // The placement request owns the hard collection link and its complete dependency closure.
                    auto read = placement->m_collection.GetAssetHandle().Read();
                    if (!read || !read->Validate())
                    {
                        fail();
                        continue;
                    }

                    operation.m_definition = *placement.Get();
                    operation.m_collection = *read.Get();
                }
                else
                {
                    auto read = IO::AssetHandle<EntityCollection>(operation.m_request.GetAssetSlot()).Read();
                    if (!read.Get() || !read->Validate())
                    {
                        fail();
                        continue;
                    }

                    operation.m_collection = *read.Get();
                }

                operation.m_hasDefinition = true;
            }

            if (!operation.m_installed)
            {
                if (!bootstrap && m_materializationsRemaining == 0)
                    continue;

                if (operation.m_concrete && operation.m_collection.m_entities.empty())
                {
                    operation.m_state = MaterializationState::kReady;
                    continue;
                }

                if (!Materialize(index, bootstrap ? nullptr : &m_materializationsRemaining))
                {
                    m_owner.CancelMaterialization({ m_token, index });
                    operation.m_state = MaterializationState::kFailed;
                    continue;
                }

                if (!operation.m_installed)
                    continue;

                // Runtime contributions have been acquired before releasing definition requests.
                operation.m_request.Reset();
                operation.m_collection = {};
                operation.m_definition = {};
            }
        }

        AdvanceLifecycle();
        for (uint32_t active = 0; active < m_activeMaterializations.size(); ++active)
        {
            const uint32_t index = m_activeMaterializations[active];
            auto& operation = *m_materializations[index];
            if (operation.m_state != MaterializationState::kPending || !operation.m_installed || operation.m_entities.empty())
                continue;

            bool failed = false;
            bool ready = true;
            for (const auto id : operation.m_entities)
            {
                const auto* entity = m_owner.Find(id);
                failed |= !entity || entity->HasFailed();
                bool wantsPublication = entity != nullptr;
                for (const Entity* ancestor = entity; ancestor; ancestor = ancestor->m_parent)
                    wantsPublication &= ancestor->m_wantsActive;

                ready &= entity && (entity->IsActive() || !wantsPublication);
            }

            if (failed)
            {
                m_owner.CancelMaterialization({ m_token, index });
                operation.m_state = MaterializationState::kFailed;
                operation.m_error = "Component lifecycle failed";
            }
            else if (ready)
            {
                operation.m_state = MaterializationState::kReady;
            }
        }

        // Preserve token status history, while polling only pending work and live memberships.
        m_activeMaterializations.erase(std::remove_if(m_activeMaterializations.begin(),
                                                      m_activeMaterializations.end(),
                                                      [&](const uint32_t index) {
                                                          const auto& operation = *m_materializations[index];
                                                          return operation.m_state > MaterializationState::kReady
                                                              || (operation.m_state == MaterializationState::kReady
                                                                  && operation.m_concrete);
                                                      }),
                                       m_activeMaterializations.end());

        if (!bootstrap && m_materializationsRemaining == 0 && m_activeMaterializations.size() > 1)
            std::rotate(m_activeMaterializations.begin(), m_activeMaterializations.begin() + 1, m_activeMaterializations.end());
    }


    EntityResources& EntityWorld::Impl::GetResources(Entity& entity)
    {
        if (!entity.m_resources)
            entity.m_resources = Memory::DefaultNew<EntityResources>(*m_assets);

        return *entity.m_resources;
    }


    EntityResidencySet& EntityWorld::Impl::ResidencyOwner(Entity& entity)
    {
        if (entity.m_residencyScope == ResidencyScope::kRegistry)
            return entity.m_registry->m_residency;

        return GetResources(entity).m_residency;
    }


    void EntityWorld::Impl::ReleaseAssets(Entity& entity, const Rtti::TypeID component, const uint64_t transition)
    {
        if (!entity.m_resources)
            return;

        auto& assets = entity.m_resources->m_assets;
        for (auto it = assets.begin(); it != assets.end();)
        {
            if (it->m_component != component || it->m_transition != transition)
            {
                ++it;
                continue;
            }

            ResidencyOwner(entity).Remove(it->m_asset, it->m_expectedType);
            it = assets.erase(it);
        }
    }


    void EntityWorld::Impl::TeardownComponent(Entity& entity, const uint32_t column, const bool destroy)
    {
        auto& chunk = *entity.m_chunk;
        const auto& info = *chunk.m_archetype.m_columns[column];
        void* data = chunk.Get(entity.m_row, column);

        auto& stage = chunk.Stage(entity.m_row, column);
        TeardownValue(entity, info, data, stage);
        if (destroy)
            info.m_type->m_destructor(data);
    }


    void EntityWorld::Impl::TeardownValue(Entity& entity, const EntityComponentInfo& info, void* data, ComponentStage& stage,
                                          const uint64_t transition)
    {
        ComponentContext context{ entity, m_owner };
        ComponentLoadingContext loading{ context };

        if ((stage & ComponentStage::kActive) != ComponentStage::kNone && info.m_deactivate)
            info.m_deactivate(data, context);
        if ((stage & ComponentStage::kInitialized) != ComponentStage::kNone && info.m_shutdown)
            info.m_shutdown(data, context);
        if ((stage & ComponentStage::kLoading) != ComponentStage::kNone && info.m_unload)
            info.m_unload(data, loading);

        ReleaseAssets(entity, info.m_type->m_id, transition);
        stage = ComponentStage::kNone;
    }


    void EntityWorld::Impl::CancelReplacements(Entity& entity, const Rtti::TypeID type, const bool keepAuthoredValues)
    {
        if (!entity.m_resources)
            return;

        auto& replacements = entity.m_resources->m_replacements;

        bool valuesChanged = false;
        for (auto it = replacements.begin(); it != replacements.end();)
        {
            if (type.IsValid() && it->m_info->m_type->m_id != type)
            {
                ++it;
                continue;
            }

            TeardownValue(entity, *it->m_info, it->m_data, it->m_stage, it->m_transition);
            if (keepAuthoredValues)
            {
                FE_Assert(!entity.m_active);

                const uint32_t column = entity.m_chunk->m_archetype.Find(it->m_info->m_type->m_id);
                FE_Assert(column != kInvalidIndex);
                TeardownComponent(entity, column, true);
                it->m_info->m_type->m_moveConstructor(entity.m_chunk->Get(entity.m_row, column), it->m_data);
                entity.m_chunk->Stage(entity.m_row, column) = ComponentStage::kNone;
                valuesChanged = true;
            }

            it->m_info->m_type->m_destructor(it->m_data);
            Memory::DefaultFree(it->m_data);
            it = replacements.erase(it);
        }

        if (valuesChanged)
        {
            MarkUnready(entity);
            MarkChanged(entity);
        }
    }


    // Compute the following node before invoking cleanup, so callbacks may destroy the visited entity.
    template<class Callable>
    static void VisitSubtreePostOrder(Entity& root, Callable&& callable)
    {
        Entity* entity = &root;
        while (entity->GetFirstChild())
            entity = entity->GetFirstChild();

        while (entity)
        {
            Entity* next = nullptr;
            if (entity != &root)
            {
                next = entity->GetNextSibling();
                if (next)
                {
                    while (next->GetFirstChild())
                        next = next->GetFirstChild();
                }
                else
                {
                    next = entity->GetParent();
                }
            }

            callable(*entity);
            entity = next;
        }
    }


    void EntityWorld::Impl::DeactivateSubtree(Entity& entity, const bool keepAuthoredValues)
    {
        VisitSubtreePostOrder(entity, [&](Entity& current) {
            DeactivateEntity(current, keepAuthoredValues);
        });
    }


    void EntityWorld::Impl::DeactivateEntity(Entity& entity, const bool keepAuthoredValues)
    {
        const auto& order = entity.m_chunk->m_archetype.m_lifecycleOrder;
        ComponentContext context{ entity, m_owner };
        for (uint32_t i = order.size(); i > 0; --i)
        {
            const uint32_t column = order[i - 1];
            auto& stage = entity.m_chunk->Stage(entity.m_row, column);
            if ((stage & ComponentStage::kActive) == ComponentStage::kNone)
                continue;

            const auto& info = *entity.m_chunk->m_archetype.m_columns[column];
            if (info.m_deactivate)
                info.m_deactivate(entity.m_chunk->Get(entity.m_row, column), context);
            stage &= ~ComponentStage::kActive;
        }

        entity.m_active = false;
        entity.m_activationPrepared = false;
        CancelReplacements(entity, Rtti::TypeID::kNull, keepAuthoredValues);
    }


    void EntityWorld::Impl::RequireAsset(Entity& entity, const IO::AssetID id, const Rtti::TypeID type)
    {
        FE_Assert(m_loading.m_component.IsValid(), "Require is legal only during dependency discovery or Load");
        if (!id.IsValid())
            return;

        auto& contributions = GetResources(entity).m_assets;
        for (const auto& contribution : contributions)
        {
            const bool sameComponent =
                contribution.m_component == m_loading.m_component && contribution.m_transition == m_loading.m_transition;
            const bool sameAsset = contribution.m_asset == id && contribution.m_expectedType == type;
            if (sameComponent && sameAsset)
                return;
        }

        ResidencyOwner(entity).Add(id, type);
        contributions.push_back({ m_loading.m_component, id, type, m_loading.m_transition });
    }


    LifecycleResult EntityWorld::Impl::LoadValue(Entity& entity, const EntityComponentInfo& info, void* data,
                                                 ComponentStage& stage, const uint64_t transition)
    {
        ComponentLoadingContext loading{ { entity, m_owner } };
        if ((stage & ComponentStage::kLoaded) != ComponentStage::kNone)
            return LifecycleResult::kSucceeded;

        m_loading.m_component = info.m_type->m_id;
        m_loading.m_transition = transition;

        auto loadingScope = festd::defer([&] {
            m_loading.m_component = Rtti::TypeID::kNull;
            m_loading.m_transition = 0;
        });

        if ((stage & ComponentStage::kDiscovered) == ComponentStage::kNone)
        {
            stage |= ComponentStage::kDiscovered;
            if (info.m_type->m_serialize)
            {
                struct Discovery
                {
                    EntityWorld* m_world;
                    Entity* m_entity;
                } discovery{ &m_owner, &entity };

                const auto result = Serialization::VisitAssetReferences(
                    *info.m_type,
                    data,
                    &discovery,
                    [](void* user, const Uuid asset, const Rtti::TypeID type, const uint32_t kind) {
                        if (kind != festd::to_underlying(IO::DependencyKind::kHard))
                            return;

                        auto& d = *static_cast<Discovery*>(user);
                        d.m_world->RequireAsset(*d.m_entity, asset, type);
                    });

                if (result != Serialization::ResultCode::kSuccess)
                    return LifecycleResult::kFailed;
            }
        }

        auto dependenciesReady = [&] {
            if (!entity.m_resources)
                return LifecycleResult::kSucceeded;

            LifecycleResult result = LifecycleResult::kSucceeded;
            for (const auto& contribution : entity.m_resources->m_assets)
            {
                if (contribution.m_component != info.m_type->m_id || contribution.m_transition != transition)
                    continue;

                const auto status = ResidencyOwner(entity).Poll(contribution.m_asset, contribution.m_expectedType);
                if (status == LifecycleResult::kFailed)
                    return status;

                if (status == LifecycleResult::kPending)
                    result = status;
            }

            return result;
        };

        auto status = dependenciesReady();
        if (status == LifecycleResult::kSucceeded)
        {
            stage |= ComponentStage::kLoading;
            status = info.m_load ? info.m_load(data, loading) : LifecycleResult::kSucceeded;
            if (status == LifecycleResult::kSucceeded)
                status = dependenciesReady();
        }

        if (status == LifecycleResult::kSucceeded)
            stage |= ComponentStage::kLoaded;

        return status;
    }


    bool EntityWorld::Impl::AdmitLifecycle(Entity& entity)
    {
        if (!m_started)
            return true;

        const uint64_t frame = m_explicitMaterializationFrames ? m_materializationFrame : m_schedule.m_epoch;
        if (m_lifecycleBudgetEpoch != frame)
        {
            m_lifecycleBudgetEpoch = frame;
            m_lifecycleRemaining = 32;
        }

        if (entity.m_lifecycleEpoch == frame)
            return true;

        if (m_lifecycleRemaining == 0)
            return false;

        --m_lifecycleRemaining;
        entity.m_lifecycleEpoch = frame;
        return true;
    }


    bool EntityWorld::Impl::PrepareSubtree(Entity& entity)
    {
        if (entity.m_failed || !entity.m_wantsActive)
            return false;

        auto& chunk = *entity.m_chunk;
        bool ownLoaded = true;
        auto failComponent = [&](const uint32_t column) {
            if (entity.m_active)
            {
                TeardownComponent(entity, column, false);
                chunk.Stage(entity.m_row, column) = ComponentStage::kFailed;
            }
            else
                entity.m_failed = true;
        };

        ComponentContext context{ entity, m_owner };
        for (const uint32_t column : chunk.m_archetype.m_lifecycleOrder)
        {
            auto& stage = chunk.Stage(entity.m_row, column);
            const auto& info = *chunk.m_archetype.m_columns[column];
            if ((stage & (ComponentStage::kLoaded | ComponentStage::kFailed)) != ComponentStage::kNone)
                continue;

            const auto status = LoadValue(entity, info, chunk.Get(entity.m_row, column), stage);
            if (status == LifecycleResult::kFailed)
            {
                failComponent(column);
                return false;
            }

            if (status == LifecycleResult::kPending)
                ownLoaded = false;
            else
                stage |= ComponentStage::kLoaded;
        }

        // Child readiness is retained between polls; only pending subtrees are revisited.
        bool areChildrenReady = true;
        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
        {
            if (!child->m_wantsActive)
                continue;

            const bool childReady = child->m_prepared || PrepareSubtree(*child);
            if (!childReady && (!entity.m_active || !child->m_failed))
                areChildrenReady = false;

            if (child->m_failed)
            {
                if (!entity.m_active)
                    entity.m_failed = true;
                else
                    UnwindSubtree(*child);
            }
        }

        const bool subtreeLoading = !ownLoaded || !areChildrenReady;
        if (entity.m_failed || (!entity.m_active && subtreeLoading))
            return false;

        const bool needsInit = eastl::any_of(chunk.m_archetype.m_lifecycleOrder.begin(),
                                             chunk.m_archetype.m_lifecycleOrder.end(),
                                             [&](const uint32_t column) {
                                                 return (chunk.Stage(entity.m_row, column)
                                                         & (ComponentStage::kInitialized | ComponentStage::kFailed))
                                                     == ComponentStage::kNone;
                                             });
        if (needsInit && !AdmitLifecycle(entity))
            return false;

        for (const uint32_t column : chunk.m_archetype.m_lifecycleOrder)
        {
            auto& stage = chunk.Stage(entity.m_row, column);
            if ((stage & (ComponentStage::kInitialized | ComponentStage::kFailed)) != ComponentStage::kNone)
                continue;

            if ((stage & ComponentStage::kLoaded) == ComponentStage::kNone)
                continue;

            const auto& info = *chunk.m_archetype.m_columns[column];
            bool dependenciesInitialized = true;
            for (const auto dependency : info.m_initAfter)
            {
                const uint32_t dependencyColumn = chunk.m_archetype.Find(dependency);
                const ComponentStage dependencyStage = chunk.Stage(entity.m_row, dependencyColumn);
                if ((dependencyStage & ComponentStage::kFailed) != ComponentStage::kNone)
                {
                    failComponent(column);
                    return false;
                }

                dependenciesInitialized &= (dependencyStage & ComponentStage::kInitialized) != ComponentStage::kNone;
            }

            if (!dependenciesInitialized)
                continue;

            // Undo is required even when a synchronous transition reports failure after partial work.
            stage |= ComponentStage::kInitialized;
            if (info.m_init && info.m_init(chunk.Get(entity.m_row, column), context) != LifecycleResult::kSucceeded)
            {
                failComponent(column);
                return false;
            }
        }

        bool ownPrepared = true;
        for (uint32_t column = 0; column < chunk.m_archetype.m_columns.size(); ++column)
        {
            const auto stage = chunk.Stage(entity.m_row, column);
            ownPrepared &= (stage & (ComponentStage::kInitialized | ComponentStage::kFailed)) != ComponentStage::kNone;
        }

        entity.m_prepared = ownPrepared && areChildrenReady;
        return entity.m_prepared;
    }


    LifecycleResult EntityWorld::Impl::ActivateSubtree(Entity& entity)
    {
        if (!entity.m_wantsActive || entity.m_failed)
            return LifecycleResult::kFailed;

        if (!entity.m_activationPrepared && !AdmitLifecycle(entity))
            return LifecycleResult::kPending;

        auto& chunk = *entity.m_chunk;
        ComponentContext context{ entity, m_owner };
        for (const uint32_t column : chunk.m_archetype.m_lifecycleOrder)
        {
            auto& stage = chunk.Stage(entity.m_row, column);
            if ((stage & ComponentStage::kInitialized) == ComponentStage::kNone)
                continue;

            if ((stage & ComponentStage::kActive) != ComponentStage::kNone)
                continue;

            const auto& info = *chunk.m_archetype.m_columns[column];
            stage |= ComponentStage::kActive;
            MarkChanged(entity);
            if (info.m_activate && info.m_activate(chunk.Get(entity.m_row, column), context) != LifecycleResult::kSucceeded)
            {
                if (entity.m_active)
                {
                    TeardownComponent(entity, column, false);
                    chunk.Stage(entity.m_row, column) = ComponentStage::kFailed;
                }
                else
                {
                    entity.m_failed = true;
                }

                return LifecycleResult::kFailed;
            }
        }

        entity.m_activationPrepared = true;
        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
        {
            if (!child->m_wantsActive)
                continue;

            if (!child->m_active && !child->m_prepared)
                continue;

            const LifecycleResult result = ActivateSubtree(*child);
            if (result == LifecycleResult::kPending)
                return LifecycleResult::kPending;

            if (result == LifecycleResult::kFailed)
            {
                if (!entity.m_active)
                    entity.m_failed = true;
                else
                    UnwindSubtree(*child);

                return LifecycleResult::kFailed;
            }
        }
        return LifecycleResult::kSucceeded;
    }


    void EntityWorld::Impl::AdvanceReplacements(Entity& entity)
    {
        if (!entity.m_active || !entity.m_wantsActive)
            return;

        if (!entity.m_resources)
            return;

        auto& replacements = entity.m_resources->m_replacements;
        for (auto it = replacements.begin(); it != replacements.end();)
        {
            auto& replacement = *it;
            const auto& info = *replacement.m_info;
            auto result = LoadValue(entity, info, replacement.m_data, replacement.m_stage, replacement.m_transition);
            if (result == LifecycleResult::kPending)
            {
                ++it;
                continue;
            }

            ComponentContext context{ entity, m_owner };
            if (result == LifecycleResult::kSucceeded)
            {
                replacement.m_stage |= ComponentStage::kInitialized;
                if (info.m_init)
                    result = info.m_init(replacement.m_data, context);
            }

            if (result == LifecycleResult::kSucceeded)
            {
                replacement.m_stage |= ComponentStage::kActive;
                MarkChanged(entity);
                if (info.m_activate)
                    result = info.m_activate(replacement.m_data, context);
            }

            if (result == LifecycleResult::kSucceeded)
            {
                const uint32_t column = entity.m_chunk->m_archetype.Find(info.m_type->m_id);
                TeardownComponent(entity, column, true);

                info.m_type->m_moveConstructor(entity.m_chunk->Get(entity.m_row, column), replacement.m_data);
                entity.m_chunk->Stage(entity.m_row, column) = replacement.m_stage;

                for (auto& contribution : entity.m_resources->m_assets)
                {
                    if (contribution.m_transition == replacement.m_transition)
                        contribution.m_transition = 0;
                }
            }
            else
            {
                TeardownValue(entity, info, replacement.m_data, replacement.m_stage, replacement.m_transition);
                Fail("Component replacement failed; previous active value retained");
            }

            info.m_type->m_destructor(replacement.m_data);
            Memory::DefaultFree(replacement.m_data);
            it = replacements.erase(it);
        }
    }


    void EntityWorld::Impl::MarkUnready(Entity& entity)
    {
        entity.m_activationPrepared = false;
        Entity* root = &entity;
        for (Entity* current = &entity; current; current = current->m_parent)
        {
            current->m_prepared = false;
            root = current;
        }

        if (festd::find(m_lifecycleWork.m_roots, root->m_id) == m_lifecycleWork.m_roots.end())
            m_lifecycleWork.m_roots.push_back(root->m_id);
    }


    void EntityWorld::Impl::UnwindSubtree(Entity& entity)
    {
        // Complete one children-first deactivation pass before tearing down any component.
        DeactivateSubtree(entity, false);
        VisitSubtreePostOrder(entity, [&](Entity& current) {
            const auto& order = current.m_chunk->m_archetype.m_lifecycleOrder;
            for (uint32_t i = order.size(); i > 0; --i)
                TeardownComponent(current, order[i - 1], false);

            current.m_failed = true;
            current.m_prepared = false;
        });
    }


    void EntityWorld::Impl::PublishSubtree(Entity& entity)
    {
        entity.m_active = true;
        MarkChanged(entity);

        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
        {
            if (child->m_prepared && child->m_wantsActive && !child->m_failed)
                PublishSubtree(*child);
        }
    }


    void EntityWorld::Impl::UnloadSubtree(Entity& entity)
    {
        VisitSubtreePostOrder(entity, [&](Entity& current) {
            const auto& order = current.m_chunk->m_archetype.m_lifecycleOrder;
            for (uint32_t i = order.size(); i > 0; --i)
                TeardownComponent(current, order[i - 1], false);

            current.m_prepared = false;
            current.m_wantsActive = false;
        });
    }


    void EntityWorld::Impl::AdvanceLifecycle()
    {
        FE_PROFILER_ZONE();

        // Replacements finish atomically before group readiness and publication are reconsidered.
        for (const EntityID id : m_lifecycleWork.m_replacements)
        {
            if (Entity* entity = m_owner.Find(id))
                AdvanceReplacements(*entity);
        }

        m_lifecycleWork.m_replacements.erase(std::remove_if(m_lifecycleWork.m_replacements.begin(),
                                                            m_lifecycleWork.m_replacements.end(),
                                                            [&](const EntityID id) {
                                                                const Entity* entity = m_owner.Find(id);
                                                                return !entity || !entity->m_resources
                                                                    || entity->m_resources->m_replacements.empty();
                                                            }),
                                             m_lifecycleWork.m_replacements.end());

        const uint32_t rootCount = m_lifecycleWork.m_roots.size();
        for (uint32_t i = 0; i < rootCount; ++i)
        {
            Entity* entity = m_owner.Find(m_lifecycleWork.m_roots[i]);
            if (!entity || entity->m_parent || !entity->m_wantsActive || entity->m_failed)
                continue;

            const bool wasActive = entity->m_active;
            if (wasActive && entity->m_prepared)
                continue;

            PrepareSubtree(*entity);
            if (entity->m_failed)
            {
                if (!wasActive)
                    UnwindSubtree(*entity);
                continue;
            }

            if (entity->m_prepared || wasActive)
            {
                const LifecycleResult result = ActivateSubtree(*entity);
                if (result == LifecycleResult::kSucceeded)
                    PublishSubtree(*entity);
                else if (result == LifecycleResult::kFailed && !wasActive)
                    UnwindSubtree(*entity);
            }
        }

        m_lifecycleWork.m_roots.erase(std::remove_if(m_lifecycleWork.m_roots.begin(),
                                                     m_lifecycleWork.m_roots.end(),
                                                     [&](const EntityID id) {
                                                         const Entity* entity = m_owner.Find(id);
                                                         return !entity || entity->m_parent || !entity->m_wantsActive
                                                             || entity->m_failed || (entity->m_active && entity->m_prepared);
                                                     }),
                                      m_lifecycleWork.m_roots.end());
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


    MaterializationToken EntityWorld::LoadEntities(EntityRegistry& registry, const EntityCollection& entities)
    {
        const auto token = m_impl->SpawnCollection(registry, entities);
        m_impl->m_materializations[token.m_index]->m_concrete = true;
        return token;
    }


    bool EntityWorld::LoadDefinition(const EntityWorldAsset& definition, festd::vector<MaterializationToken>* operations)
    {
        return m_impl->LoadDefinition(definition, operations);
    }


    bool EntityWorld::Impl::LoadDefinition(const EntityWorldAsset& definition, festd::vector<MaterializationToken>* operations)
    {
        FE_PROFILER_ZONE();
        FE_Assert(!m_schedule.IsCollecting() && !m_schedule.IsExecuting());
        FE_Assert(m_storage.m_registries.empty(), "Definitions require an empty world");

        if (!definition.Validate())
            return false;

        for (const auto& group : definition.m_registries)
        {
            EntityRegistry& registry = CreateRegistry(group.m_key);
            const auto concrete = m_owner.LoadEntities(registry, group.m_entities);
            if (operations)
                operations->push_back(concrete);

            for (const auto& placement : group.m_placements)
            {
                const auto token = m_owner.LoadPlacement(registry, placement.GetAssetID());
                if (operations)
                    operations->push_back(token);
            }
        }

        return true;
    }


    bool EntityWorld::CaptureSnapshot(EntityWorldSnapshotAsset& snapshot) const
    {
        return m_impl->CaptureSnapshot(snapshot);
    }


    bool EntityWorld::Impl::CaptureSnapshot(EntityWorldSnapshotAsset& snapshot)
    {
        FE_PROFILER_ZONE();
        FE_Assert(Threading::IsMainThread());
        FE_Assert(!m_schedule.IsUpdating(), "Snapshot capture requires a closed update epoch");

        std::lock_guard lock{ m_pendingCommands.m_lock };
        if (!m_pendingCommands.m_lists.empty())
            return Fail("Snapshot blocked by pending entity commands");

        for (const auto* operation : m_materializations)
        {
            if (operation->m_state == MaterializationState::kPending)
                return Fail("Snapshot blocked by pending entity materialization");
        }

        EntityWorldSnapshotAsset result;
        for (EntityRegistry* registry : m_storage.m_registries)
        {
            EntityWorldRegistrySnapshot group;
            group.m_key = registry->GetKey();

            festd::vector<const Entity*> pending;
            for (const auto& slot : m_storage.m_slots)
            {
                if (slot.m_entity && slot.m_entity->m_registry == registry && !slot.m_entity->m_parent)
                    pending.push_back(slot.m_entity);
            }

            // A preorder walk preserves sibling order while avoiding recursion for deep hierarchies.
            while (!pending.empty())
            {
                const Entity* entity = pending.back();
                pending.pop_back();
                for (const Entity* child = entity->m_lastChild; child; child = child->m_previousSibling)
                    pending.push_back(child);

                if (entity->m_resources && !entity->m_resources->m_replacements.empty())
                    return Fail("Snapshot blocked by pending component replacements");

                bool wantsPublication = true;
                for (const Entity* ancestor = entity; ancestor; ancestor = ancestor->m_parent)
                    wantsPublication &= ancestor->m_wantsActive;

                if (wantsPublication && !entity->IsActive())
                    return Fail("Snapshot blocked by unsettled entity lifecycle");

                EntityRecord record;
                record.m_uuid = entity->m_uuid;
                record.m_parentUuid = entity->m_parent ? entity->m_parent->m_uuid : Uuid::kNull;
                record.m_name.assign(entity->m_name.c_str() ? entity->m_name.c_str() : "", entity->m_name.size());
                record.m_active = entity->m_wantsActive;
                record.m_residency = entity->m_residencyScope;
                for (uint32_t column = 0; column < entity->m_chunk->m_archetype.m_columns.size(); ++column)
                {
                    const auto& info = *entity->m_chunk->m_archetype.m_columns[column];
                    if (info.m_policy.m_transient)
                        continue;

                    Rtti::Any& component = record.m_components.emplace_back();
                    if (!info.m_type->m_serialize || !info.m_type->m_deserialize || !info.m_type->m_copyConstructor
                        || !component.Emplace(*info.m_type)
                        || !CopyComponent(*info.m_type, entity->m_chunk->Get(entity->m_row, column), component.GetValue()))
                    {
                        return Fail("Invalid serializable entity component");
                    }
                }

                group.m_entities.m_entities.push_back(std::move(record));
            }

            // Keep original bindings, including deleted source rows, to prevent accidental source re-expansion.
            for (const auto* operation : m_materializations)
            {
                if (!operation->m_placement || operation->m_registry != registry
                    || operation->m_state != MaterializationState::kReady)
                {
                    continue;
                }

                EntityPlacementSnapshot placement;
                placement.m_asset = operation->m_asset;
                placement.m_rootUuid = m_owner.Find(operation->m_root)->GetUuid();
                placement.m_bindings = operation->m_bindings;
                for (const auto id : operation->m_entities)
                {
                    if (const auto* entity = m_owner.Find(id))
                        placement.m_members.push_back(entity->GetUuid());
                }

                group.m_placements.push_back(std::move(placement));
            }

            result.m_registries.push_back(std::move(group));
        }

        snapshot = std::move(result);
        m_error = {};
        return true;
    }


    bool EntityWorld::RestoreSnapshot(const EntityWorldSnapshotAsset& snapshot, festd::vector<MaterializationToken>* operations)
    {
        return m_impl->RestoreSnapshot(snapshot, operations);
    }


    bool EntityWorld::Impl::RestoreSnapshot(const EntityWorldSnapshotAsset& snapshot,
                                            festd::vector<MaterializationToken>* operations)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Threading::IsMainThread());
        FE_Assert(!m_schedule.IsCollecting() && !m_schedule.IsExecuting());
        FE_Assert(m_storage.m_registries.empty(), "Snapshots require an empty world");

        if (!snapshot.Validate())
            return false;

        for (const auto& group : snapshot.m_registries)
        {
            EntityRegistry& registry = CreateRegistry(group.m_key);

            const auto token = m_owner.LoadEntities(registry, group.m_entities);
            if (operations)
                operations->push_back(token);

            // Materialize concrete rows now, before installing ownership records. Lifecycle may remain asynchronous.
            if (!Materialize(token.m_index))
            {
                while (!m_storage.m_registries.empty())
                    m_owner.RemoveRegistry(*m_storage.m_registries.back());

                return false;
            }

            auto& concrete = *m_materializations[token.m_index];
            concrete.m_collection = {};
            if (concrete.m_entities.empty())
                concrete.m_state = MaterializationState::kReady;

            for (const auto& placement : group.m_placements)
            {
                auto* operation = Memory::DefaultNew<EntityMaterialization>();
                operation->m_registry = &registry;
                operation->m_registryId = registry.GetID();
                operation->m_asset = placement.m_asset;
                operation->m_placement = true;
                operation->m_state = MaterializationState::kReady;
                operation->m_root = m_owner.Find(placement.m_rootUuid, false)->GetID();
                operation->m_bindings = placement.m_bindings;
                for (const Uuid uuid : placement.m_members)
                    operation->m_entities.push_back(m_owner.Find(uuid, false)->GetID());

                m_placements[operation->m_asset] = m_materializations.size();
                m_activeMaterializations.push_back(m_materializations.size());
                m_materializations.push_back(operation);
            }
        }

        return true;
    }
} // namespace FE::Framework
