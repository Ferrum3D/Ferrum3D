#include <Core/Threading/Thread.h>
#include <Framework/Entities/EntityRuntime.h>

namespace FE::Framework
{
    namespace
    {
        std::atomic<uint32_t> GNextWorld{ 1 };
        EntityAssetServices GDefaultAssets;
    } // namespace


    EntityWorld::EntityWorld(EntityAssetServices* assets, void* services)
        : m_impl(Memory::DefaultNew<Impl>())
    {
        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        const uint32_t token = GNextWorld.fetch_add(1);
        FE_Assert(token <= 0xffff, "World incarnation space exhausted");
        m_impl->m_token = static_cast<uint16_t>(token);
        m_impl->m_assets = assets ? assets : &GDefaultAssets;
        m_impl->m_services = services;
    }


    EntityWorld::~EntityWorld()
    {
        m_impl->ClearEpoch();
        for (uint32_t i = m_impl->m_systems.size(); i > 0; --i)
            m_impl->m_systems[i - 1]->Shutdown(*this);
        while (!m_impl->m_registries.empty())
            RemoveRegistry(*m_impl->m_registries.back());
        for (auto* commands : m_impl->m_commands)
            Memory::DefaultDelete(commands);
        for (auto* archetype : m_impl->m_archetypes)
            Memory::DefaultDelete(archetype);

        for (auto* operation : m_impl->m_materializations)
            Memory::DefaultDelete(operation);
        Memory::DefaultDelete(m_impl);
    }


    bool EntityWorld::Fail(const festd::ascii_view message)
    {
        m_impl->m_error = message;
        return false;
    }


    Entity* EntityWorld::Find(const EntityID id) const
    {
        if (id.m_value == 0 || id.World() != m_impl->m_token || id.Slot() >= m_impl->m_slots.size())
            return nullptr;

        const auto& slot = m_impl->m_slots[id.Slot()];
        return slot.m_generation == id.Generation() ? slot.m_entity : nullptr;
    }


    Entity* EntityWorld::Find(const Uuid uuid, const bool activeOnly) const
    {
        const auto it = m_impl->m_uuidLookup.find(uuid);
        if (it == m_impl->m_uuidLookup.end())
            return nullptr;
        return !activeOnly || it->second->IsActive() ? it->second : nullptr;
    }


    EntityRegistry& EntityWorld::CreateRegistry()
    {
        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_Assert(!m_impl->m_collecting && !m_impl->m_executing);

        void* storage = Memory::DefaultAllocate(sizeof(EntityRegistry), alignof(EntityRegistry));
        auto* registry = ::new (storage) EntityRegistry(*this, *m_impl->m_assets, m_impl->m_nextRegistryId++);
        m_impl->m_registries.push_back(registry);
        return *registry;
    }


    void EntityWorld::RemoveRegistry(EntityRegistry& registry)
    {
        FE_Assert(Threading::IsMainThread(), "Entity world safe points must execute on the main thread");
        FE_Assert(!m_impl->m_executing && !m_impl->m_collecting);

        const auto registryIt = festd::find(m_impl->m_registries, &registry);
        if (registryIt == m_impl->m_registries.end())
            return;

        registry.m_unloading = true;
        for (uint32_t index = 0; index < m_impl->m_materializations.size(); ++index)
        {
            const auto& operation = *m_impl->m_materializations[index];
            if (operation.m_registry == &registry && operation.m_state <= MaterializationState::kReady)
                CancelMaterialization({ m_impl->m_token, index });
        }
        for (auto& slot : m_impl->m_slots)
        {
            if (slot.m_entity && slot.m_entity->m_registry == &registry && !slot.m_entity->m_parent)
                DestroyEntity(*slot.m_entity);
        }
        // Cancel every pending list that names this owner before releasing its address.
        {
            std::lock_guard lock{ m_impl->m_commandLock };
            for (auto it = m_impl->m_commands.begin(); it != m_impl->m_commands.end();)
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
                    it = m_impl->m_commands.erase(it);
                }
                else
                    ++it;
            }
        }
        m_impl->m_registries.erase(registryIt);

        Memory::DefaultDelete(&registry);
    }


    EntityComponentRegistry& EntityWorld::Components()
    {
        return m_impl->m_components;
    }


    uint64_t EntityWorld::GetEpoch() const
    {
        return m_impl->m_epoch;
    }


    uint64_t EntityWorld::GetHierarchyRevision() const
    {
        return m_impl->m_hierarchyRevision;
    }


    festd::ascii_view EntityWorld::GetLastError() const
    {
        return m_impl->m_error;
    }


    uint32_t EntityWorld::GetEntityCount() const
    {
        return static_cast<uint32_t>(m_impl->m_uuidLookup.size());
    }


    uint32_t EntityWorld::GetChunkCount() const
    {
        return m_impl->m_chunks.size();
    }


    Entity* EntityWorld::AllocateEntity(EntityRegistry& registry, const Env::Name name, const Uuid uuid)
    {
        if (m_impl->m_uuidLookup.contains(uuid))
            return nullptr;

        uint32_t index;
        if (m_impl->m_freeSlots.empty())
        {
            index = m_impl->m_slots.size();
            FE_Assert(index <= 0xffffff, "Entity slot space exhausted");
            m_impl->m_slots.push_back({});
        }
        else
        {
            index = m_impl->m_freeSlots.back();
            m_impl->m_freeSlots.pop_back();
        }
        auto& slot = m_impl->m_slots[index];
        Entity* entity = ::new (m_impl->m_entities.AllocateMemory())
            Entity(*this, registry, EntityID::Pack(m_impl->m_token, index, slot.m_generation), uuid, name);
        slot.m_entity = entity;
        m_impl->m_uuidLookup.emplace(uuid, entity);
        return entity;
    }


    void EntityWorld::Reparent(Entity& entity, Entity* parent)
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
        entity.m_previousSibling = parent ? parent->m_lastChild : nullptr;
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
        FE_Assert(m_impl->m_hierarchyRevision != UINT64_MAX, "Hierarchy revision exhausted");
        ++m_impl->m_hierarchyRevision;
        MarkChanged(entity);
    }


    void EntityWorld::DestroyEntity(Entity& entity)
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
            m_impl->m_chunks.erase(festd::find(m_impl->m_chunks, chunk));

            Memory::DefaultDelete(chunk);
        }
        entity.m_chunk = nullptr;
        Reparent(entity, nullptr);
        m_impl->m_uuidLookup.erase(entity.m_uuid);
        auto& slot = m_impl->m_slots[entity.m_id.Slot()];
        slot.m_entity = nullptr;
        if (slot.m_generation < 0xffffff)
        {
            ++slot.m_generation;
            m_impl->m_freeSlots.push_back(entity.m_id.Slot());
        }
        entity.~Entity();
        m_impl->m_entities.GetAllocator()->deallocate(&entity, sizeof(Entity), alignof(Entity));
        FE_Assert(m_impl->m_hierarchyRevision != UINT64_MAX, "Hierarchy revision exhausted");
        ++m_impl->m_hierarchyRevision;
    }


    void EntityWorld::Migrate(Entity& entity, const festd::span<const EntityComponentInfo* const> columns,
                              const festd::span<void* const> values)
    {
        Archetype* archetype = nullptr;
        // Compare canonical type lists, never trust a signature hash as identity.
        for (auto* candidate : m_impl->m_archetypes)
        {
            if (candidate->m_columns.size() != columns.size())
                continue;

            bool equal = true;
            for (uint32_t i = 0; i < columns.size(); ++i)
                equal &= candidate->m_columns[i] == columns[i];
            if (equal)
            {
                archetype = candidate;
                break;
            }
        }
        if (!archetype)
        {
            archetype = Memory::DefaultNew<Archetype>(columns);
            m_impl->m_archetypes.push_back(archetype);
        }

        FE_Assert(archetype->m_lifecycleOrder.size() == columns.size(), "Missing or cyclic component initialization dependency");
        ArchetypeChunk* destination = nullptr;
        for (auto* chunk : m_impl->m_chunks)
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
            m_impl->m_chunks.push_back(destination);
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
                if (!actualValues[i] || oldColumn == kInvalidIndex || !(source->Stage(oldRow, oldColumn) & kActive))
                    continue;

                CancelReplacements(entity, type.m_id);
                void* data = Memory::DefaultAllocate(type.m_size, type.m_alignment);
                type.m_moveConstructor(data, actualValues[i]);
                entity.m_runtime->m_replacements.push_back({ columns[i], data, m_impl->m_nextTransition++ });
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
                type.m_moveConstructor(destination->Get(row, i), actualValues[i]);
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

        FE_Assert(m_impl->m_structureRevision != UINT64_MAX, "Structure revision exhausted");
        ++m_impl->m_structureRevision;
        entity.m_chunk = destination;
        entity.m_row = row;
        MarkUnready(entity);
        MarkChanged(entity);

        if (source)
        {
            source->Free(oldRow);
            if (source->m_count == 0)
            {
                m_impl->m_chunks.erase(festd::find(m_impl->m_chunks, source));

                Memory::DefaultDelete(source);
            }
        }
    }


    void EntityWorld::SetReparentHandler(const ReparentHandler handler)
    {
        FE_Assert(Threading::IsMainThread() && !m_impl->m_executing && !m_impl->m_collecting);
        FE_Assert(!handler || !m_impl->m_reparentHandler || handler == m_impl->m_reparentHandler);
        m_impl->m_reparentHandler = handler;
    }


    uint64_t EntityWorld::NextChangeVersion()
    {
        if (m_impl->m_changeVersion == UINT64_MAX)
        {
            FE_Assert(m_impl->m_changeEra != UINT64_MAX, "Change version era exhausted");
            ++m_impl->m_changeEra;
            m_impl->m_changeVersion = 0;
            for (auto* chunk : m_impl->m_chunks)
            {
                for (auto& version : chunk->m_versions)
                    version = 0;
            }
        }
        return ++m_impl->m_changeVersion;
    }


    void EntityWorld::MarkChanged(Entity& entity)
    {
        if (!entity.m_chunk)
            return;

        std::lock_guard guard(m_impl->m_versionLock);
        const uint64_t version = NextChangeVersion();
        for (auto& column : entity.m_chunk->m_versions)
            column = version;
    }


    void* EntityWorld::LookupComponent(const Entity& entity, const Rtti::TypeID type, const bool write) const
    {
        FE_Assert(!m_impl->m_collecting, "System collection cannot inspect live component data");
        const auto* callback = m_impl->m_executing
            ? static_cast<const CallbackContext*>(Threading::FiberRuntimeInfo::Get().m_executionContext)
            : nullptr;
        const auto* traversal = callback && callback->m_world == this ? callback->m_traversal : nullptr;
        if (traversal)
        {
            bool declared = false;
            for (const auto& access : traversal->m_accesses)
            {
                const Entity* source = access.m_parent ? callback->m_entity->GetParent() : callback->m_entity;
                if (access.m_type == type && (!write || access.m_write) && source == &entity)
                    declared = true;
            }
            FE_AssertDebug(declared, "Entity component lookup exceeds traversal access declarations");
        }
        if (!entity.m_chunk)
            return nullptr;

        const uint32_t column = entity.m_chunk->m_archetype.Find(type);
        if (column == kInvalidIndex)
            return nullptr;
        if (traversal && !(entity.m_chunk->Stage(entity.m_row, column) & kActive))
            return nullptr;
        return entity.m_chunk->Get(entity.m_row, column);
    }
} // namespace FE::Framework
