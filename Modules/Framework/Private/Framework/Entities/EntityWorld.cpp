#include <Core/IO/MemoryStream.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Threading/Thread.h>
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
        FE_Assert(!m_schedule.m_collecting && !m_schedule.m_executing);

        if (!key.IsValid())
            key = Uuid::Random();

        FE_Assert(!m_owner.FindRegistry(key), "Registry key is already owned");
        void* storage = Memory::DefaultAllocate(sizeof(EntityRegistry), alignof(EntityRegistry));
        auto* registry = ::new (storage) EntityRegistry(m_owner, *m_assets, m_nextRegistryId++, key);
        m_storage.m_registries.push_back(registry);
        return *registry;
    }


    void EntityWorld::Impl::RemoveRegistry(EntityRegistry& registry)
    {
        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_Assert(!m_schedule.m_executing && !m_schedule.m_collecting);

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
                    if (command.m_registry == &registry)
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
        Memory::DefaultDelete(&registry);
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
        chunk->Free(entity.m_row);

        if (chunk->m_count == 0)
        {
            m_storage.m_chunks.erase(festd::find(m_storage.m_chunks, chunk));
            Memory::DefaultDelete(chunk);
        }

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


    void EntityWorld::Impl::Migrate(Entity& entity, const festd::span<const EntityComponentInfo* const> columns,
                                    const festd::span<void* const> values)
    {
        FE_PROFILER_ZONE();

        Archetype* archetype = GetArchetype(columns);
        FE_Assert(archetype);

        ArchetypeChunk* destination = nullptr;
        for (auto* chunk : m_storage.m_chunks)
        {
            if (&chunk->m_archetype == archetype && &chunk->m_registry == entity.m_registry
                && chunk->m_count < archetype->m_capacity)
            {
                destination = chunk;
                break;
            }
        }

        if (!destination)
        {
            destination = Memory::DefaultNew<ArchetypeChunk>(*archetype, *entity.m_registry);
            m_storage.m_chunks.push_back(destination);
        }

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
                actualValues[i] = nullptr;
            }
        }

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
        {
            source->Free(oldRow);

            if (source->m_count == 0)
            {
                m_storage.m_chunks.erase(festd::find(m_storage.m_chunks, source));
                Memory::DefaultDelete(source);
            }
        }
    }


    void EntityWorld::SetReparentHandler(const ReparentHandler handler)
    {
        FE_Assert(Threading::IsMainThread() && !m_impl->m_schedule.m_executing && !m_impl->m_schedule.m_collecting);
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
        FE_Assert(!m_schedule.m_collecting, "System collection cannot inspect live component data");

        const auto* callback = m_schedule.m_executing
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


    bool EntityWorld::RequireAsset(Entity& entity, IO::AssetID id, Rtti::TypeID type)
    {
        return m_impl->RequireAsset(entity, id, type);
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
        FE_Assert(Threading::IsMainThread() && !m_impl->m_schedule.m_updating);
        FE_Assert(festd::find(m_impl->m_services, &service) == m_impl->m_services.end());
        m_impl->m_services.push_back(&service);
        service.Init(*this);
    }


    void EntityWorld::RemoveService(WorldService& service)
    {
        FE_Assert(Threading::IsMainThread() && !m_impl->m_schedule.m_updating && GetEntityCount() == 0);
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


    void EntityWorld::BeginUpdate()
    {
        m_impl->m_schedule.BeginUpdate();
    }


    bool EntityWorld::EndUpdate()
    {
        return m_impl->m_schedule.EndUpdate();
    }


    void* EntityWorld::AllocateTraversal(size_t size, size_t alignment)
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


    bool EntityWorld::SchedulePhase(Phase phase, festd::span<const Rc<WaitGroup>> prerequisites)
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
    EntityRegistry& EntityWorld::CreateRegistry(Uuid key)
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


    void* EntityWorld::LookupComponent(const Entity& entity, Rtti::TypeID type, bool write) const
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


    MaterializationToken EntityWorld::SpawnCollection(EntityRegistry& registry, IO::AssetID collection)
    {
        return m_impl->SpawnCollection(registry, collection);
    }


    MaterializationToken EntityWorld::LoadPlacement(EntityRegistry& registry, IO::AssetID placement)
    {
        return m_impl->LoadPlacement(registry, placement);
    }


    MaterializationToken EntityWorld::LoadPlacement(EntityRegistry& registry, IO::AssetID placement,
                                                    const EntityCollectionInstanceAsset& definition,
                                                    const EntityCollection& collection)
    {
        return m_impl->LoadPlacement(registry, placement, definition, collection);
    }


    void EntityWorld::CancelMaterialization(MaterializationToken token)
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
        return { operation.m_state, operation.m_root, operation.m_error };
    }


    festd::span<const EntityUuidBinding> EntityWorld::GetMaterializationBindings(const MaterializationToken token) const
    {
        if (token.m_world != m_impl->m_token || token.m_index >= m_impl->m_materializations.size())
            return {};

        return m_impl->m_materializations[token.m_index]->m_bindings;
    }


    namespace
    {
        bool DestructionContains(const EntityWorld& world, const Command& command, const EntityID target)
        {
            if (command.m_kind != CommandKind::kDestroy)
                return false;

            for (const Entity* current = world.Find(target); current; current = current->GetParent())
            {
                if (current->GetID() == command.m_target.m_id)
                    return true;
            }

            return false;
        }


        const EntityRegistry* GetCommandRegistry(const EntityWorld& world, const Command& command)
        {
            if (command.m_registry)
                return command.m_registry;

            const Entity* entity = world.Find(command.m_target.m_id);
            return entity ? &entity->GetRegistry() : nullptr;
        }


        bool CommandsConflict(const EntityWorld& world, const Command& first, const Command& second)
        {
            if (first.m_target.m_id.m_value && first.m_target.m_id == second.m_target.m_id)
                return true;

            if (DestructionContains(world, first, second.m_target.m_id)
                || DestructionContains(world, second, first.m_target.m_id))
            {
                return true;
            }

            if (DestructionContains(world, first, second.m_parent.m_id)
                || DestructionContains(world, second, first.m_parent.m_id))
            {
                return true;
            }

            if (first.m_kind == CommandKind::kParent && second.m_kind == CommandKind::kParent)
            {
                if (first.m_parent.m_id.m_value && first.m_parent.m_id == second.m_target.m_id)
                    return true;

                if (second.m_parent.m_id.m_value && second.m_parent.m_id == first.m_target.m_id)
                    return true;
            }

            if (first.m_kind != CommandKind::kUnloadRegistry && second.m_kind != CommandKind::kUnloadRegistry)
                return false;

            const EntityRegistry* firstRegistry = GetCommandRegistry(world, first);
            return firstRegistry && firstRegistry == GetCommandRegistry(world, second);
        }


        bool CommandListsConflict(const EntityWorld& world, const festd::span<const Command> first,
                                  const festd::span<const Command> second)
        {
            for (const auto& firstCommand : first)
            {
                for (const auto& secondCommand : second)
                {
                    if (CommandsConflict(world, firstCommand, secondCommand))
                        return true;
                }
            }

            return false;
        }
    } // namespace


    void EntityWorld::Impl::Submit(EntityCommandList&& commands)
    {
        FE_PROFILER_ZONE();

        FE_Assert(commands.m_impl && commands.m_impl->m_world == &m_owner);

        std::lock_guard lock{ m_pendingCommands.m_lock };

        auto& list = *commands.m_impl;
        list.m_eligibleEpoch = m_schedule.m_epoch;

        bool hasPublication = false;
        bool hasHierarchyEdits = false;
        for (const auto& command : list.m_commands)
        {
            hasPublication |= command.m_kind == CommandKind::kCreate || command.m_kind == CommandKind::kComponent;
            hasHierarchyEdits |= command.m_kind == CommandKind::kParent;
        }

        // Preserve detach/destroy recording order when a hierarchy batch also needs deferred publication.
        if (!hasPublication || hasHierarchyEdits)
        {
            if (hasPublication)
                ++list.m_eligibleEpoch;
            m_pendingCommands.m_lists.push_back(std::exchange(commands.m_impl, nullptr));
            return;
        }

        festd::inline_vector<EntityID> deferredTargets;
        festd::inline_vector<EntityID> destroyedTargets;
        for (const auto& command : list.m_commands)
        {
            if (command.m_kind == CommandKind::kComponent && command.m_target.m_id.m_value)
                deferredTargets.push_back(command.m_target.m_id);
            if (command.m_kind == CommandKind::kDestroy && command.m_target.m_id.m_value)
                destroyedTargets.push_back(command.m_target.m_id);
        }

        // Independent existing targets can commit without waiting for creations.
        auto* immediate = Memory::DefaultNew<EntityCommandList::Impl>(m_owner);
        immediate->m_id = list.m_id;
        immediate->m_eligibleEpoch = m_schedule.m_epoch;

        festd::vector<Command> deferred;
        for (const auto& command : list.m_commands)
        {
            const bool destroyed = festd::find(destroyedTargets, command.m_target.m_id) != destroyedTargets.end();

            const bool needsPublication =
                !destroyed && festd::find(deferredTargets, command.m_target.m_id) != deferredTargets.end();

            const bool tokenCommand = command.m_target.m_token.m_list || command.m_parent.m_token.m_list;
            const bool isImmediate = command.m_target.m_id.m_value && !tokenCommand && !needsPublication;

            if (destroyed && command.m_kind == CommandKind::kComponent)
            {
                command.m_component->m_type->m_destructor(command.m_payload);
                continue;
            }

            if (isImmediate)
                immediate->m_commands.push_back(command);
            else
                deferred.push_back(command);
        }


        list.m_commands = std::move(deferred);
        list.m_eligibleEpoch = m_schedule.m_epoch + 1;
        if (!immediate->m_commands.empty())
            m_pendingCommands.m_lists.push_back(immediate);
        else
            Memory::DefaultDelete(immediate);

        m_pendingCommands.m_lists.push_back(std::exchange(commands.m_impl, nullptr));
    }


    bool EntityWorld::Impl::CommitImpl(const bool bootstrap)
    {
        FE_PROFILER_ZONE();

        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_Assert(!m_schedule.m_collecting && !m_schedule.m_executing, "Structural commit requires a safe boundary");

        m_error = {};

        // Detach eligible batches under the submission lock; validation and callbacks run after releasing it.
        festd::vector<EntityCommandList::Impl*> ready;
        {
            std::lock_guard lock{ m_pendingCommands.m_lock };
            for (auto it = m_pendingCommands.m_lists.begin(); it != m_pendingCommands.m_lists.end();)
            {
                if (!bootstrap && (*it)->m_eligibleEpoch > m_schedule.m_epoch)
                {
                    ++it;
                    continue;
                }
                ready.push_back(*it);
                it = m_pendingCommands.m_lists.erase(it);
            }
        }

        auto cleanup = festd::defer([&] {
            for (auto* list : ready)
                Memory::DefaultDelete(list);
        });

        festd::vector<bool> conflicts(ready.size(), false);
        for (uint32_t first = 0; first < ready.size(); ++first)
        {
            for (uint32_t second = first + 1; second < ready.size(); ++second)
            {
                if (ready[first]->m_id == ready[second]->m_id)
                    continue;

                if (CommandListsConflict(m_owner, ready[first]->m_commands, ready[second]->m_commands))
                    conflicts[first] = conflicts[second] = true;
            }
        }

        bool success = true;
        for (uint32_t listIndex = 0; listIndex < ready.size(); ++listIndex)
        {
            auto& list = *ready[listIndex];
            if (conflicts[listIndex])
            {
                success = Fail("Independent command lists conflict on the same target");
                continue;
            }

            struct Value
            {
                const EntityComponentInfo* m_info;
                void* m_value;
            };

            struct Edit
            {
                Entity* m_entity = nullptr;
                EntityRegistry* m_registry = nullptr;
                Uuid m_uuid = Uuid::kNull;
                Env::Name m_name;
                uint32_t m_parent = kInvalidIndex;
                ResidencyScope m_residency = ResidencyScope::kEntity;
                bool m_created = false;
                bool m_destroy = false;
                bool m_unload = false;
                bool m_wantsActive = true;
                bool m_touched = false;
                bool m_componentsChanged = false;
                bool m_parentChanged = false;
                festd::vector<Value> m_values;
                bool m_hasValues = false;

                void PrepareValues()
                {
                    if (m_hasValues)
                        return;

                    m_hasValues = true;
                    if (!m_entity)
                        return;

                    for (const auto* info : m_entity->m_chunk->m_archetype.m_columns)
                        m_values.push_back({ info, nullptr });
                }
            };

            festd::vector<Edit> edits;
            festd::vector<uint32_t> tokens(list.m_created, kInvalidIndex);
            festd::vector<EntityRegistry*> unloadRegistries;
            for (const auto& slot : m_storage.m_slots)
            {
                if (!slot.m_entity)
                    continue;

                const Entity& entity = *slot.m_entity;
                Edit edit;
                edit.m_entity = slot.m_entity;
                edit.m_registry = entity.m_registry;
                edit.m_uuid = entity.m_uuid;
                edit.m_name = entity.m_name;
                edit.m_wantsActive = entity.m_wantsActive;
                edits.push_back(std::move(edit));
            }

            auto entityIndex = [&](Entity* entity) {
                if (!entity)
                    return kInvalidIndex;

                for (uint32_t i = 0; i < edits.size(); ++i)
                {
                    if (edits[i].m_entity == entity)
                        return i;
                }

                return kInvalidIndex;
            };

            for (auto& edit : edits)
                edit.m_parent = entityIndex(edit.m_entity->m_parent);

            auto resolve = [&](const EntityTarget& target) {
                if (target.m_token.m_list)
                {
                    if (target.m_token.m_list != list.m_id || target.m_token.m_index >= tokens.size())
                        return kInvalidIndex;

                    return tokens[target.m_token.m_index];
                }
                return entityIndex(m_owner.Find(target.m_id));
            };

            struct EditContext
            {
                festd::vector<Edit>* m_edits;
                EntityCommandList::Impl* m_list;
            } editContext{ &edits, &list };

            auto component = [](void* data, const uint32_t index, Rtti::TypeID type, const bool write) -> void* {
                auto& context = *static_cast<EditContext*>(data);
                if (index >= context.m_edits->size())
                    return nullptr;

                auto& edit = (*context.m_edits)[index];
                edit.PrepareValues();

                auto value = festd::find_if(edit.m_values.begin(), edit.m_values.end(), [&](const Value& item) {
                    return item.m_info->m_type->m_id == type;
                });
                if (value == edit.m_values.end())
                    return nullptr;

                if (value->m_value)
                    return value->m_value;

                void* source = edit.m_entity ? edit.m_entity->FindComponent(type) : nullptr;
                if (!write)
                    return source;

                const auto& rtti = *value->m_info->m_type;
                if (!source || !rtti.m_copyConstructor)
                    return nullptr;

                void* storage = context.m_list->m_arena.allocate(rtti.m_size, rtti.m_alignment);
                if (!storage)
                {
                    storage = Memory::DefaultAllocate(rtti.m_size, rtti.m_alignment);
                    context.m_list->m_largePayloads.push_back(storage);
                }

                rtti.m_copyConstructor(storage, source);
                context.m_list->m_ownedValues.push_back({ value->m_info, storage });
                value->m_value = storage;
                edit.m_componentsChanged = true;
                return storage;
            };

            // Build an isolated edit snapshot first; rejected topology or content cannot mutate live rows.
            bool valid = true;
            for (const auto& command : list.m_commands)
            {
                if (command.m_kind == CommandKind::kCreate)
                {
                    if (festd::find(m_storage.m_registries, command.m_registry) == m_storage.m_registries.end())
                    {
                        valid = Fail("Invalid registry");
                        break;
                    }

                    if (command.m_registry->GetID() != command.m_registryId)
                    {
                        valid = Fail("Invalid registry");
                        break;
                    }

                    for (const auto& edit : edits)
                    {
                        if (edit.m_uuid == command.m_uuid)
                            valid = Fail("Duplicate entity UUID");
                    }

                    Edit edit;
                    edit.m_registry = command.m_registry;
                    edit.m_uuid = command.m_uuid;
                    edit.m_name = command.m_name;
                    edit.m_residency = command.m_residency;
                    edit.m_created = edit.m_touched = true;
                    tokens[command.m_target.m_token.m_index] = edits.size();

                    edits.push_back(std::move(edit));
                    continue;
                }

                if (command.m_kind == CommandKind::kUnloadRegistry)
                {
                    if (festd::find(m_storage.m_registries, command.m_registry) == m_storage.m_registries.end())
                        valid = Fail("Invalid registry");
                    else if (command.m_registry->GetID() != command.m_registryId)
                        valid = Fail("Invalid registry");
                    else
                        unloadRegistries.push_back(command.m_registry);

                    continue;
                }

                const uint32_t index = resolve(command.m_target);
                if (index == kInvalidIndex)
                {
                    valid = Fail("Stale entity ID or invalid list-local token");
                    break;
                }

                auto& edit = edits[index];
                if (edit.m_destroy)
                    continue;

                edit.m_touched = true;
                switch (command.m_kind)
                {
                case CommandKind::kDestroy:
                    edit.m_destroy = true;
                    for (uint32_t descendant = 0; descendant < edits.size(); ++descendant)
                    {
                        uint32_t ancestor = edits[descendant].m_parent;
                        uint32_t depth = 0;
                        while (ancestor != kInvalidIndex && depth++ < edits.size())
                        {
                            if (ancestor == index)
                            {
                                edits[descendant].m_destroy = true;
                                edits[descendant].m_touched = true;
                                break;
                            }
                            ancestor = edits[ancestor].m_parent;
                        }
                    }
                    break;

                case CommandKind::kRename:
                    edit.m_name = command.m_name;
                    break;

                case CommandKind::kActive:
                    edit.m_wantsActive = command.m_active;
                    break;

                case CommandKind::kUnload:
                    edit.m_unload = true;
                    edit.m_wantsActive = false;
                    break;

                case CommandKind::kParent:
                    {
                        const bool nullParent = !command.m_parent.m_id.m_value && !command.m_parent.m_token.m_list;
                        const uint32_t parent = nullParent ? kInvalidIndex : resolve(command.m_parent);

                        if (!nullParent && parent == kInvalidIndex)
                        {
                            valid = Fail("Invalid parent handle");
                        }
                        else if (parent != kInvalidIndex && edits[parent].m_registry != edit.m_registry)
                        {
                            valid = Fail("Parenting cannot cross registry or world boundaries");
                        }
                        else
                        {
                            if (edit.m_parent != parent && m_reparentHandler
                                && command.m_reparentMode == ReparentMode::kPreserveWorld)
                            {
                                ReparentContext context{ index,
                                                         parent,
                                                         (edits.size()),
                                                         command.m_reparentMode,
                                                         &editContext,
                                                         [](void* data, uint32_t target) {
                                                             return (*static_cast<EditContext*>(data)->m_edits)[target].m_parent;
                                                         },
                                                         component };
                                if (!m_reparentHandler(context))
                                {
                                    valid = Fail("Invalid reparent transform");
                                    break;
                                }
                            }

                            edit.m_parent = parent;
                            edit.m_parentChanged = true;
                        }
                        break;
                    }

                case CommandKind::kComponent:
                case CommandKind::kRemove:
                    {
                        edit.PrepareValues();

                        const auto found = festd::find_if(edit.m_values.begin(), edit.m_values.end(), [&](const Value& value) {
                            return value.m_info->m_type->m_id == command.m_type;
                        });
                        if (found != edit.m_values.end())
                        {
                            const auto companions = found->m_info->m_runtimeCompanions;
                            edit.m_values.erase(found);
                            if (command.m_kind == CommandKind::kRemove)
                            {
                                for (const auto companion : companions)
                                {
                                    const bool stillRequired =
                                        std::any_of(edit.m_values.begin(), edit.m_values.end(), [companion](const Value& value) {
                                            const auto& owned = value.m_info->m_runtimeCompanions;
                                            return festd::find(owned, companion) != owned.end();
                                        });
                                    if (stillRequired)
                                        continue;

                                    const auto runtime = festd::find_if(edit.m_values.begin(),
                                                                        edit.m_values.end(),
                                                                        [companion](const Value& value) {
                                                                            return value.m_info->m_type->m_id == companion;
                                                                        });
                                    if (runtime != edit.m_values.end())
                                        edit.m_values.erase(runtime);
                                }
                            }
                        }

                        if (command.m_kind == CommandKind::kComponent)
                        {
                            edit.m_values.push_back({ command.m_component, command.m_payload });
                            // Add companions with their authored component, preserving later explicit removals in command order.
                            for (const auto companion : command.m_component->m_runtimeCompanions)
                            {
                                const auto runtime =
                                    festd::find_if(edit.m_values.begin(), edit.m_values.end(), [companion](const Value& value) {
                                        return value.m_info->m_type->m_id == companion;
                                    });
                                if (runtime == edit.m_values.end())
                                    edit.m_values.push_back({ m_components.Find(companion), nullptr });
                            }
                        }
                        edit.m_componentsChanged = true;
                        break;
                    }
                default:
                    break;
                }

                if (!valid)
                    break;
            }

            for (uint32_t i = 0; valid && i < edits.size(); ++i)
            {
                uint32_t ancestor = edits[i].m_parent;
                if (ancestor != kInvalidIndex && edits[ancestor].m_destroy && !edits[i].m_destroy)
                {
                    valid = Fail("Cannot parent an entity to a target queued for destruction");
                    break;
                }

                uint32_t depth = 0;
                while (ancestor != kInvalidIndex)
                {
                    if (ancestor == i || ++depth > edits.size())
                    {
                        valid = Fail("Invalid entity hierarchy");
                        break;
                    }

                    ancestor = edits[ancestor].m_parent;
                }

                auto& edit = edits[i];
                if (!edit.m_touched || edit.m_destroy)
                    continue;

                if (festd::find(unloadRegistries, edit.m_registry) != unloadRegistries.end())
                {
                    valid = Fail("Cannot edit and unload the same registry in one batch");
                    break;
                }

                if (!edit.m_created && !edit.m_componentsChanged)
                    continue;

                festd::sort(edit.m_values.begin(), edit.m_values.end(), [](const Value& a, const Value& b) {
                    return a.m_info->m_type->m_id < b.m_info->m_type->m_id;
                });

                festd::vector<const EntityComponentInfo*> columns;
                for (const auto& value : edit.m_values)
                    columns.push_back(value.m_info);

                if (!GetArchetype(columns))
                    valid = Fail("Missing or cyclic component initialization dependency");
            }

            if (!valid)
            {
                success = false;
                continue;
            }

            // No entity/link/storage mutation precedes validation of the complete list.
            for (auto& edit : edits)
            {
                if (edit.m_created && !edit.m_destroy)
                {
                    edit.m_entity = AllocateEntity(*edit.m_registry, edit.m_name, edit.m_uuid);
                    edit.m_entity->m_residencyScope = edit.m_residency;
                }
            }

            for (auto& edit : edits)
            {
                if (!edit.m_touched || !edit.m_entity || edit.m_destroy)
                    continue;

                Entity& entity = *edit.m_entity;
                entity.m_name = edit.m_name;
                if (edit.m_created || edit.m_componentsChanged)
                {
                    festd::vector<const EntityComponentInfo*> columns;
                    festd::vector<void*> values;
                    for (const auto& value : edit.m_values)
                    {
                        columns.push_back(value.m_info);
                        values.push_back(value.m_value);
                    }

                    Migrate(entity, columns, values);
                    entity.m_failed = false;
                }

                if (entity.m_wantsActive != edit.m_wantsActive)
                    MarkUnready(entity);

                entity.m_wantsActive = edit.m_wantsActive;
            }

            // Detach changed links first so a valid final topology cannot temporarily form a cycle.
            for (auto& edit : edits)
            {
                if (edit.m_parentChanged && edit.m_entity)
                    Reparent(*edit.m_entity, nullptr);
            }

            for (auto& edit : edits)
            {
                if (edit.m_parentChanged && edit.m_entity && edit.m_parent != kInvalidIndex)
                    Reparent(*edit.m_entity, edits[edit.m_parent].m_entity);
            }

            for (auto& edit : edits)
            {
                Entity* entity = m_owner.Find(edit.m_uuid, false);
                if (!entity || !edit.m_touched)
                    continue;

                if (edit.m_destroy)
                {
                    DestroyEntity(*entity);
                    continue;
                }

                if (!edit.m_wantsActive)
                    DeactivateSubtree(*entity);

                if (edit.m_unload)
                {
                    // Unload keeps allocated identity but resets runtime state, descendants first.
                    UnloadSubtree(*entity);
                }
            }

            for (auto* registry : unloadRegistries)
                m_owner.RemoveRegistry(*registry);
        }

        AdvanceMaterializations(bootstrap);
        return success;
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

        operation.m_collection = collection;
        operation.m_hasDefinition = true;

        const MaterializationToken token{ m_token, (m_materializations.size()) };
        m_materializations.push_back(Memory::DefaultNew<EntityMaterialization>(std::move(operation)));
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

        for (uint32_t index = 0; index < m_materializations.size(); ++index)
        {
            const auto& operation = *m_materializations[index];
            if (operation.m_placement && operation.m_asset == placement && operation.m_state <= MaterializationState::kReady)
                return { m_token, index };
        }

        auto token = SpawnCollection(registry, EntityCollection{});
        auto& operation = *m_materializations[token.m_index];
        operation.m_placement = true;
        operation.m_hasDefinition = false;
        operation.m_asset = placement;
        if (operation.m_state == MaterializationState::kPending)
            operation.m_request = IO::AssetManager::LoadAsset(IO::Link<EntityCollectionInstanceAsset>(placement));

        return token;
    }


    MaterializationToken EntityWorld::Impl::LoadPlacement(EntityRegistry& registry, const IO::AssetID placement,
                                                          const EntityCollectionInstanceAsset& definition,
                                                          const EntityCollection& collection)
    {
        FE_Assert(Threading::IsMainThread());

        for (uint32_t index = 0; index < m_materializations.size(); ++index)
        {
            const auto& operation = *m_materializations[index];
            if (operation.m_placement && operation.m_asset == placement && operation.m_state <= MaterializationState::kReady)
                return { m_token, index };
        }

        auto token = SpawnCollection(registry, collection);
        auto& operation = *m_materializations[token.m_index];
        operation.m_placement = true;
        operation.m_asset = placement;
        operation.m_definition = definition;
        return token;
    }


    void EntityWorld::Impl::CancelMaterialization(const MaterializationToken token)
    {
        FE_Assert(Threading::IsMainThread());
        FE_Assert(!m_schedule.m_collecting && !m_schedule.m_executing);
        if (token.m_world != m_token || token.m_index >= m_materializations.size())
            return;

        auto& operation = *m_materializations[token.m_index];
        if (operation.m_state == MaterializationState::kCanceled)
            return;

        operation.m_state = MaterializationState::kCanceled;
        operation.m_request.Reset();
        operation.m_collectionRequest.Reset();
        for (const auto id : operation.m_entities)
        {
            if (auto* entity = m_owner.Find(id))
                DestroyEntity(*entity);
        }

        operation.m_entities.clear();
        operation.m_collection = {};
        operation.m_definition = {};
        operation.m_root = {};
    }


    bool EntityWorld::Impl::Materialize(uint32_t index)
    {
        FE_PROFILER_ZONE();

        auto& operation = *m_materializations[index];
        const auto& collection = operation.m_collection;

        // Validate the complete authored envelope before allocating any runtime membership.

        auto reject = [&](const festd::ascii_view error = "Invalid entity asset") {
            operation.m_error = error;
            operation.m_state = MaterializationState::kFailed;
            return false;
        };

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

        const Uuid rootUuid = operation.m_placement ? operation.m_definition.m_rootUuid : Uuid::Random();
        if (!operation.m_concrete && m_owner.Find(rootUuid, false))
            return reject("Placement root UUID is already owned");

        for (const auto& binding : operation.m_bindings)
        {
            if (m_owner.Find(binding.m_entityUuid, false))
                return reject("Concrete entity UUID is already owned");
        }

        struct SourceRecord
        {
            const EntityRecord* m_record;
            const EntityCollection* m_definition;
        };
        festd::vector<SourceRecord> records;

        EntityRecord emptyRoot;
        emptyRoot.m_uuid = rootUuid;
        if (!operation.m_concrete)
        {
            records.push_back({ operation.m_placement ? &operation.m_definition.m_root.m_entities.front() : &emptyRoot,
                                operation.m_placement ? &operation.m_definition.m_root : &collection });
        }

        for (const auto& entity : collection.m_entities)
            records.push_back({ &entity, &collection });

        festd::vector<Archetype*> schemas;
        for (const auto& source : records)
        {
            festd::vector<const EntityComponentInfo*> columns;
            for (const auto& component : source.m_record->m_components)
            {
                const auto* info = m_components.Find(component.m_type);
                if (!info || info->m_policy.m_transient || !info->m_type->m_deserialize || !info->m_type->m_defaultConstructor)
                    return reject();

                if (component.m_version != info->m_type->m_serializationVersion
                    || component.m_schemaHash != info->m_type->m_serializationSchemaHash)
                {
                    return reject();
                }

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

            schemas.push_back(archetype);
        }

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

        for (uint32_t row = 0; row < records.size(); ++row)
        {
            const auto& record = *records[row].m_record;
            Entity* entity =
                AllocateEntity(*operation.m_registry,
                               Env::Name(record.m_name),
                               row == 0 && !operation.m_concrete ? rootUuid : remap(&operation.m_bindings, record.m_uuid));
            entity->m_wantsActive = false;
            entity->m_residencyScope = record.m_residency;

            const auto& columns = schemas[row]->m_columns;
            festd::vector<void*> values(columns.size(), nullptr);
            Migrate(*entity, columns, values);
            operation.m_entities.push_back(entity->m_id);

            for (const auto& component : record.m_components)
            {
                const auto* info = m_components.Find(component.m_type);
                const auto& definition = *records[row].m_definition;

                IO::ReadOnlyMemoryStream stream(definition.m_payload.data() + component.m_payloadOffset, component.m_payloadSize);
                Serialization::PackedBinaryFormat format;
                Serialization::DeserializationContext context(&stream, format);
                context.SetObjectReferenceRemapper(&operation.m_bindings, remap);

                void* destination = entity->FindComponent(component.m_type);
                if (context.Load(*info->m_type, destination) != Serialization::ResultCode::kSuccess
                    || stream.Tell() != stream.Length())
                {
                    return reject();
                }

                // Envelope ownership remains independent of the definition's acquisition and generation pin.
                m_loading.m_component = component.m_type;
                for (const auto& dependency : component.m_dependencies)
                {
                    if (dependency.m_kind == IO::DependencyKind::kHard)
                        RequireAsset(*entity, dependency.m_asset, dependency.m_expectedType);
                }

                m_loading.m_component = Rtti::TypeID::kNull;
                const uint32_t column = entity->m_chunk->m_archetype.Find(component.m_type);
                entity->m_chunk->Stage(entity->m_row, column) |= ComponentStage::kDiscovered;
            }
        }

        operation.m_root = operation.m_entities.empty() ? EntityID{} : operation.m_entities.front();
        for (uint32_t row = operation.m_concrete ? 0 : 1; row < records.size(); ++row)
        {
            const Uuid parent = records[row].m_record->m_parentUuid;
            Entity* parentEntity = parent.IsValid() ? m_owner.Find(remap(&operation.m_bindings, parent), false)
                                                    : (operation.m_concrete ? nullptr : m_owner.Find(operation.m_root));
            Reparent(*m_owner.Find(operation.m_entities[row]), parentEntity);
        }

        for (uint32_t row = 0; row < operation.m_entities.size(); ++row)
            m_owner.Find(operation.m_entities[row])->m_wantsActive = records[row].m_record->m_active;

        for (const auto id : operation.m_entities)
        {
            Entity& entity = *m_owner.Find(id);
            for (uint32_t column = 0; column < entity.m_chunk->m_archetype.m_columns.size(); ++column)
            {
                const auto& info = *entity.m_chunk->m_archetype.m_columns[column];
                auto& stage = entity.m_chunk->Stage(entity.m_row, column);
                if (LoadValue(entity, info, entity.m_chunk->Get(entity.m_row, column), stage) == LifecycleResult::kFailed)
                    return reject();
            }
        }

        success = true;
        return true;
    }


    void EntityWorld::Impl::AdvanceMaterializations(const bool bootstrap)
    {
        FE_PROFILER_ZONE();

        const uint32_t operationCount = m_materializations.size();
        for (uint32_t index = 0; index < operationCount; ++index)
        {
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
                m_owner.CancelMaterialization({ m_token, index });
                operation.m_state = MaterializationState::kFailed;
                operation.m_error = "Entity asset unavailable";
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
                    if (!operation.m_collectionRequest.IsValid())
                    {
                        auto read = IO::AssetHandle<EntityCollectionInstanceAsset>(operation.m_request.GetAssetSlot()).Read();
                        if (!read.Get())
                        {
                            fail();
                            continue;
                        }

                        operation.m_definition = *read.Get();
                        operation.m_collectionRequest = IO::AssetManager::LoadAsset(operation.m_definition.m_collection);
                    }

                    if (!operation.m_collectionRequest.IsCompleted())
                        continue;

                    if (operation.m_collectionRequest.GetResult() != IO::AssetLoadResult::kSucceeded)
                    {
                        fail();
                        continue;
                    }

                    auto read = IO::AssetHandle<EntityCollection>(operation.m_collectionRequest.GetAssetSlot()).Read();
                    if (!read.Get())
                    {
                        fail();
                        continue;
                    }

                    operation.m_collection = *read.Get();
                }
                else
                {
                    auto read = IO::AssetHandle<EntityCollection>(operation.m_request.GetAssetSlot()).Read();
                    if (!read.Get())
                    {
                        fail();
                        continue;
                    }

                    operation.m_collection = *read.Get();
                }

                operation.m_hasDefinition = true;
            }

            if (operation.m_entities.empty())
            {
                if (operation.m_concrete && operation.m_collection.m_entities.empty())
                {
                    operation.m_state = MaterializationState::kReady;
                    continue;
                }

                if (!Materialize(index))
                {
                    operation.m_request.Reset();
                    operation.m_collectionRequest.Reset();
                    operation.m_collection = {};
                    operation.m_definition = {};
                    continue;
                }

                // Runtime contributions have been acquired before releasing definition requests.
                operation.m_request.Reset();
                operation.m_collectionRequest.Reset();
                operation.m_collection = {};
                operation.m_definition = {};
            }
        }

        AdvanceLifecycle();
        for (uint32_t index = 0; index < m_materializations.size(); ++index)
        {
            auto& operation = *m_materializations[index];
            if (operation.m_state != MaterializationState::kPending || operation.m_entities.empty())
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
    }


    namespace
    {
        // Serialize authored fields only to visit references; no redundant payload buffers are produced.
        struct DependencyOnlyFormat final : Serialization::SerializationFormat
        {
            DependencyOnlyFormat()
                : SerializationFormat(Serialization::Format::kPackedBinary)
            {
            }


            void ResetImpl() override {}


            Serialization::ResultCode BeginStoreDocumentImpl(Rtti::TypeID, uint32_t, uint64_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginLoadDocumentImpl(Rtti::TypeID, uint32_t, uint64_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndStoreDocumentImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndLoadDocumentImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginStoreObjectImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginLoadObjectImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndStoreObjectImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndLoadObjectImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginStoreFieldImpl(festd::ascii_view, uint64_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginLoadFieldImpl(festd::ascii_view, uint64_t, bool&) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndStoreFieldImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndLoadFieldImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginStoreArrayImpl(uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginLoadArrayImpl(uint32_t&) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndStoreArrayImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndLoadArrayImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginStoreElementImpl(uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode BeginLoadElementImpl(uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndStoreElementImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode EndLoadElementImpl() override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode StoreScalarImpl(Serialization::ScalarKind, const void*, uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode LoadScalarImpl(Serialization::ScalarKind, void*, uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode StoreBytesImpl(const void*, uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode LoadBytesImpl(void*, uint32_t) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode StoreStringImpl(festd::string_view) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode LoadStringSizeImpl(uint32_t&) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            Serialization::ResultCode LoadStringImpl(festd::span<char>) override
            {
                return Serialization::ResultCode::kSuccess;
            }


            uint64_t GetStoreCurrentOffsetImpl() const override
            {
                return 0;
            }


            uint64_t GetLoadCurrentOffsetImpl() const override
            {
                return 0;
            }
        };
    } // namespace


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


    void EntityWorld::Impl::DeactivateSubtree(Entity& entity, const bool keepAuthoredValues)
    {
        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
            DeactivateSubtree(*child, keepAuthoredValues);

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
        CancelReplacements(entity, Rtti::TypeID::kNull, keepAuthoredValues);
    }


    bool EntityWorld::Impl::RequireAsset(Entity& entity, const IO::AssetID id, const Rtti::TypeID type)
    {
        FE_Assert(m_loading.m_component.IsValid(), "Require is legal only during dependency discovery or Load");
        if (!id.IsValid())
            return true;

        auto& contributions = GetResources(entity).m_assets;
        for (const auto& contribution : contributions)
        {
            const bool sameComponent =
                contribution.m_component == m_loading.m_component && contribution.m_transition == m_loading.m_transition;
            const bool sameAsset = contribution.m_asset == id && contribution.m_expectedType == type;
            if (sameComponent && sameAsset)
                return true;
        }

        ResidencyOwner(entity).Add(id, type);
        contributions.push_back({ m_loading.m_component, id, type, m_loading.m_transition });
        return true;
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

                IO::WriteOnlyMemoryStream stream;
                DependencyOnlyFormat format;
                Serialization::SerializationContext serialization(&stream,
                                                                  format,
                                                                  &discovery,
                                                                  [](void* user, Uuid asset, Rtti::TypeID type, uint32_t kind) {
                                                                      if (kind != festd::to_underlying(IO::DependencyKind::kHard))
                                                                          return;

                                                                      auto& d = *static_cast<Discovery*>(user);
                                                                      d.m_world->RequireAsset(*d.m_entity, asset, type);
                                                                  });

                if (serialization.Store(*info.m_type, data) != Serialization::ResultCode::kSuccess)
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


    bool EntityWorld::Impl::PrepareSubtree(Entity& entity)
    {
        if (entity.m_failed || !entity.m_wantsActive)
            return false;

        auto& chunk = *entity.m_chunk;
        bool ownLoaded = true;
        auto failComponent = [&](uint32_t column) {
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


    bool EntityWorld::Impl::ActivateSubtree(Entity& entity)
    {
        if (!entity.m_wantsActive || entity.m_failed)
            return false;

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

                return false;
            }
        }

        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
        {
            if (!child->m_wantsActive)
                continue;

            if ((child->m_active || child->m_prepared) && !ActivateSubtree(*child))
            {
                if (!entity.m_active)
                    entity.m_failed = true;
                else
                    UnwindSubtree(*child);

                return false;
            }
        }
        return true;
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
        for (Entity* current = &entity; current; current = current->m_parent)
            current->m_prepared = false;
    }


    void EntityWorld::Impl::UnwindSubtree(Entity& entity)
    {
        DeactivateSubtree(entity, false);
        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
            UnwindSubtree(*child);

        const auto& order = entity.m_chunk->m_archetype.m_lifecycleOrder;
        for (uint32_t i = order.size(); i > 0; --i)
            TeardownComponent(entity, order[i - 1], false);

        entity.m_failed = true;
        entity.m_prepared = false;
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
        // Children release their dependencies before the parent's runtime state is reset.
        for (Entity* child = entity.m_firstChild; child; child = child->m_nextSibling)
            UnloadSubtree(*child);

        const auto& order = entity.m_chunk->m_archetype.m_lifecycleOrder;
        for (uint32_t i = order.size(); i > 0; --i)
            TeardownComponent(entity, order[i - 1], false);

        entity.m_prepared = false;
        entity.m_wantsActive = false;
    }


    void EntityWorld::Impl::AdvanceLifecycle()
    {
        FE_PROFILER_ZONE();

        // Replacements finish atomically before group readiness and publication are reconsidered.
        for (auto& slot : m_storage.m_slots)
        {
            if (slot.m_entity)
                AdvanceReplacements(*slot.m_entity);
        }

        for (auto& slot : m_storage.m_slots)
        {
            Entity* entity = slot.m_entity;
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
                if (ActivateSubtree(*entity))
                    PublishSubtree(*entity);
                else if (!wasActive)
                    UnwindSubtree(*entity);
            }
        }
    }


    EntityRegistry* EntityWorld::FindRegistry(Uuid key) const
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
        FE_Assert(!m_schedule.m_collecting && !m_schedule.m_executing);
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
        FE_Assert(!m_schedule.m_updating, "Snapshot capture requires a closed update epoch");
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

                    if (!group.m_entities.CookComponent(record, *info.m_type, entity->m_chunk->Get(entity->m_row, column)))
                        return Fail("Invalid serializable entity component");
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
        FE_Assert(!m_schedule.m_collecting && !m_schedule.m_executing);
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

                m_materializations.push_back(operation);
            }
        }

        return true;
    }
} // namespace FE::Framework
