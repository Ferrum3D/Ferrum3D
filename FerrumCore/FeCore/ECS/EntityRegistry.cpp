#include <FeCore/ECS/ArchetypeChunk.h>
#include <FeCore/ECS/EntityQuery.h>
#include <FeCore/ECS/EntityRegistry.h>

namespace FE::ECS
{
    bool EntityRegistry::AddComponent(Entity entity, const ComponentType& componentType)
    {
        FE_ASSERT_MSG(IsValid(entity), "Entity was invalid");
        auto componentTypes = GetComponentTypes(entity);
        if (componentTypes.Contains(componentType))
        {
            return false;
        }

        auto newArchetype = EntityArchetypeBuilder(componentTypes).AddComponentType(componentType).Build();

        int16_t selectedArchetype = -1;
        for (uint32_t i = 0; i < m_Archetypes.size(); ++i)
        {
            if (newArchetype == m_Archetypes[i])
            {
                selectedArchetype = static_cast<int16_t>(i);
                break;
            }
        }

        if (selectedArchetype == -1)
        {
            selectedArchetype = static_cast<int16_t>(m_Archetypes.size());
            m_Archetypes.push_back(newArchetype);
        }

        MoveToArchetype(entity, selectedArchetype);
        return true;
    }

    void EntityRegistry::AddComponent(ArraySlice<Entity> entities, const ComponentType& componentType)
    {
        // TODO: optimize this: entities can be grouped into archetypes
        for (auto& entity : entities)
        {
            AddComponent(entity, componentType);
        }
    }

    bool EntityRegistry::RemoveComponent(Entity entity, const ComponentType& componentType)
    {
        FE_ASSERT_MSG(IsValid(entity), "Entity was invalid");
        auto componentTypesSlice = GetComponentTypes(entity);
        eastl::vector<ComponentType> componentTypes;
        componentTypes.assign(componentTypesSlice.begin(), componentTypesSlice.end());
        auto componentIter = eastl::find(componentTypes.begin(), componentTypes.end(), componentType);

        componentTypes.erase(componentIter);
        if (componentTypes.empty())
        {
            MoveToArchetype(entity, -1);
            return true;
        }

        auto newArchetype = EntityArchetypeBuilder(componentTypes).Build();

        int16_t selectedArchetype = -1;
        for (uint32_t i = 0; i < m_Archetypes.size(); ++i)
        {
            if (newArchetype == m_Archetypes[i])
            {
                selectedArchetype = static_cast<int16_t>(i);
                break;
            }
        }

        if (selectedArchetype == -1)
        {
            selectedArchetype = static_cast<int16_t>(m_Archetypes.size());
            m_Archetypes.push_back(newArchetype);
        }

        MoveToArchetype(entity, selectedArchetype);
        return true;
    }

    void EntityRegistry::MoveToArchetype(Entity entity, int16_t archetypeIndex)
    {
        auto& srcData = m_Data[entity.GetID()];
        EntityData data{};
        data.ArchetypeID = archetypeIndex;

        if (archetypeIndex >= 0)
        {
            FE_ASSERT(m_Archetypes[archetypeIndex].CreateEntity(data.EntityID, &data.Chunk) == ECSResult::Success);

            if (!srcData.IsEmpty())
            {
                for (auto& type : m_Archetypes[archetypeIndex].ComponentTypes())
                {
                    // If some components won't be found in either chunk, it will just return ComponentNotFoundError,
                    // so we do not check if the component exists
                    srcData.Chunk->CopyComponentToChunk(srcData.EntityID, data.EntityID, type.Type, data.Chunk);
                }
            }
        }

        if (!srcData.IsEmpty())
        {
            FE_ASSERT(m_Archetypes[srcData.ArchetypeID].DestroyEntity(srcData.EntityID, srcData.Chunk) == ECSResult::Success);
        }

        m_Data[entity.GetID()] = data;
    }

    void EntityRegistry::RemoveComponent(ArraySlice<Entity> entities, const ComponentType& componentType)
    {
        for (auto& entity : entities)
        {
            RemoveComponent(entity, componentType);
        }
    }

    Entity EntityRegistry::CreateEntity()
    {
        auto id = GetEntityID();
        m_Data.Emplace(id);
        uint32_t version = 0;
        if (m_VersionTable.size() < id + 1)
        {
            m_VersionTable.resize(id + 1, 0);
        }

        version = m_VersionTable[id];

        return Entity::Create(id, version);
    }

    Entity EntityRegistry::CreateEntity(ArraySlice<ComponentType> componentTypes)
    {
        auto archetype = EntityArchetypeBuilder(componentTypes).Build();

        int16_t selectedArchetype = -1;
        for (uint32_t i = 0; i < m_Archetypes.size(); ++i)
        {
            if (archetype == m_Archetypes[i])
            {
                selectedArchetype = static_cast<int16_t>(i);
                break;
            }
        }

        if (selectedArchetype == -1)
        {
            selectedArchetype = static_cast<int16_t>(m_Archetypes.size());
            m_Archetypes.push_back(archetype);
        }

        auto entity = CreateEntity();
        MoveToArchetype(entity, selectedArchetype);
        return entity;
    }

    Entity EntityRegistry::CreateEntity(EntityArchetype* archetype)
    {
        auto entity = CreateEntity();
        auto selectedArchetypeIter = eastl::find(m_Archetypes.begin(), m_Archetypes.end(), *archetype);
        auto selectedArchetype = static_cast<int16_t>(selectedArchetypeIter - m_Archetypes.begin());

        FE_ASSERT_MSG(selectedArchetype != -1, "The archetype was not created from the registry");

        MoveToArchetype(entity, selectedArchetype);
        return entity;
    }

    Entity EntityRegistry::CloneEntity(Entity entity)
    {
        auto& srcData = m_Data[entity.GetID()];
        auto result = CreateEntity(&m_Archetypes[srcData.ArchetypeID]);
        auto& dstData = m_Data[result.GetID()];

        for (auto& type : m_Archetypes[srcData.ArchetypeID].ComponentTypes())
        {
            srcData.Chunk->CopyComponentToChunk(srcData.EntityID, dstData.EntityID, type.Type, dstData.Chunk);
        }

        return result;
    }

    void EntityRegistry::CreateEntities(ArraySliceMut<Entity> entities)
    {
        for (auto& entity : entities)
        {
            entity = CreateEntity();
        }
    }

    void EntityRegistry::CreateEntities(ArraySlice<ComponentType> componentTypes, ArraySliceMut<Entity> entities)
    {
        auto archetype = EntityArchetypeBuilder(componentTypes).Build();

        int16_t selectedArchetype = -1;
        for (uint32_t i = 0; i < m_Archetypes.size(); ++i)
        {
            if (archetype == m_Archetypes[i])
            {
                selectedArchetype = static_cast<int16_t>(i);
                break;
            }
        }

        if (selectedArchetype == -1)
        {
            selectedArchetype = static_cast<int16_t>(m_Archetypes.size());
            m_Archetypes.push_back(archetype);
        }

        for (auto& entity : entities)
        {
            entity = CreateEntity();
            MoveToArchetype(entity, selectedArchetype);
        }
    }

    void EntityRegistry::CreateEntities(EntityArchetype* archetype, ArraySliceMut<Entity> entities)
    {
        auto selectedArchetypeIter = eastl::find(m_Archetypes.begin(), m_Archetypes.end(), *archetype);
        auto selectedArchetype = static_cast<int16_t>(selectedArchetypeIter - m_Archetypes.begin());

        FE_ASSERT_MSG(selectedArchetype != -1, "The archetype was not created from the registry");

        for (auto& entity : entities)
        {
            entity = CreateEntity();
            MoveToArchetype(entity, selectedArchetype);
        }
    }

    void EntityRegistry::CloneEntity(Entity entity, ArraySliceMut<Entity> entities)
    {
        for (auto& e : entities)
        {
            e = CloneEntity(entity);
        }
    }

    void EntityRegistry::UpdateEntityQuery(EntityQuery* query)
    {
        query->m_Archetypes.clear();
        for (auto& archetype : m_Archetypes)
        {
            if ((query->m_IncludeNone.ComponentTypeCount() == 0
                 || archetype.Match(query->m_IncludeNone) == EntityArchetypeMatch::None)
                && (query->m_IncludeAll.ComponentTypeCount() == 0
                    || archetype.Match(query->m_IncludeAll) == EntityArchetypeMatch::All)
                && (query->m_IncludeAny.ComponentTypeCount() == 0
                    || archetype.Match(query->m_IncludeAny) == EntityArchetypeMatch::Some))
            {
                query->m_Archetypes.push_back(&archetype);
            }
        }
    }

    void EntityRegistry::DestroyEntity(Entity entity)
    {
        auto& data = m_Data[entity.GetID()];
        ++m_VersionTable[entity.GetID()];
        if (!data.IsEmpty())
        {
            m_Archetypes[data.ArchetypeID].DestroyEntity(data.EntityID, data.Chunk);
        }
        m_Data.Remove(entity.GetID());
        m_FreeList.push_back(entity.GetID());
    }

    void EntityRegistry::DestroyEntities(ArraySlice<Entity> entities)
    {
        for (auto& entity : entities)
        {
            DestroyEntity(entity);
        }
    }

    ArraySlice<ComponentType> EntityRegistry::GetComponentTypes(Entity entity)
    {
        auto& data = m_Data[entity.GetID()];
        if (data.IsEmpty())
        {
            return {};
        }

        return m_Archetypes[data.ArchetypeID].ComponentTypes();
    }

    bool EntityRegistry::HasComponent(Entity entity, const ComponentType& componentType)
    {
        return GetComponentTypes(entity).Contains(componentType);
    }

    bool EntityRegistry::CopyComponent(Entity entity, const ComponentType& componentType, void* destination)
    {
        auto& data = m_Data[entity.GetID()];
        return m_Archetypes[data.ArchetypeID].CopyComponent(data.EntityID, data.Chunk, componentType.Type, destination)
            == ECSResult::Success;
    }

    bool EntityRegistry::UpdateComponent(Entity entity, const ComponentType& componentType, const void* source)
    {
        auto& data = m_Data[entity.GetID()];
        return m_Archetypes[data.ArchetypeID].UpdateComponent(data.EntityID, data.Chunk, componentType.Type, source)
            == ECSResult::Success;
    }
} // namespace FE::ECS
