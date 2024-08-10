#pragma once
#include <FeCore/Containers/SparseSet.h>
#include <FeCore/ECS/Entity.h>
#include <FeCore/ECS/EntityArchetype.h>

namespace FE::ECS
{
    class EntityQuery;

    class EntityRegistry final : public Memory::RefCountedObjectBase
    {
        struct EntityData
        {
            ArchetypeChunk* Chunk = nullptr;
            int16_t ArchetypeID = -1;
            uint16_t EntityID = 0;

            [[nodiscard]] inline bool IsEmpty() const
            {
                return Chunk == nullptr;
            }
        };

        SparseSet<EntityID, EntityData> m_Data;
        eastl::vector<EntityID> m_VersionTable;
        eastl::vector<EntityArchetype> m_Archetypes;

        eastl::vector<EntityID> m_FreeList;

        EntityID m_EntityID = 0;
        inline EntityID GetEntityID()
        {
            if (!m_FreeList.empty())
            {
                const auto result = m_FreeList.back();
                m_FreeList.pop_back();
                return result;
            }

            return m_EntityID++;
        }

        void MoveToArchetype(Entity entity, int16_t archetypeIndex);

    public:
        FE_RTTI_Class(EntityRegistry, "1A7583F3-7C99-4CDF-9C41-27B303C82134");

        inline EntityRegistry() = default;
        inline ~EntityRegistry() override = default;

        inline EntityID GetCurrentEntityVersion(Entity entity)
        {
            return m_VersionTable[entity.GetID()];
        }

        inline bool IsValid(Entity entity)
        {
            return m_Data.Contains(entity.GetID()) && GetCurrentEntityVersion(entity) == entity.GetVersion();
        }

        bool AddComponent(Entity entity, const ComponentType& componentType);
        void AddComponent(ArraySlice<Entity> entities, const ComponentType& componentType);

        template<class T>
        inline bool AddComponent(Entity entity)
        {
            return AddComponent(entity, ComponentType::Create<T>());
        }

        template<class T>
        inline void AddComponent(ArraySlice<Entity> entities)
        {
            AddComponent(entities, ComponentType::Create<T>());
        }

        template<class T>
        inline bool AddComponent(Entity entity, const T& data)
        {
            auto result = AddComponent<T>(entity);
            SetComponent<T>(entity, data);
            return result;
        }

        bool RemoveComponent(Entity entity, const ComponentType& componentType);
        void RemoveComponent(ArraySlice<Entity> entities, const ComponentType& componentType);

        template<class T>
        inline bool RemoveComponent(Entity entity)
        {
            return RemoveComponent(entity, ComponentType::Create<T>());
        }

        template<class T>
        inline bool RemoveComponent(ArraySlice<Entity> entities)
        {
            return RemoveComponent(entities, ComponentType::Create<T>());
        }

        Entity CreateEntity();
        Entity CreateEntity(ArraySlice<ComponentType> componentTypes);
        Entity CreateEntity(EntityArchetype* archetype);

        Entity CloneEntity(Entity entity);

        void CreateEntities(ArraySliceMut<Entity> entities);
        void CreateEntities(ArraySlice<ComponentType> componentTypes, ArraySliceMut<Entity> entities);
        void CreateEntities(EntityArchetype* archetype, ArraySliceMut<Entity> entities);

        void CloneEntity(Entity entity, ArraySliceMut<Entity> entities);

        void UpdateEntityQuery(EntityQuery* query);

        void DestroyEntity(Entity entity);
        void DestroyEntities(ArraySlice<Entity> entities);

        inline uint32_t GetComponentCount(Entity entity)
        {
            return static_cast<uint32_t>(GetComponentTypes(entity).Length());
        }

        ArraySlice<ComponentType> GetComponentTypes(Entity entity);
        bool HasComponent(Entity entity, const ComponentType& componentType);

        template<class T>
        inline bool HasComponent(Entity entity)
        {
            return HasComponent(entity, ComponentType::Create<T>());
        }

        bool CopyComponent(Entity entity, const ComponentType& componentType, void* destination);

        template<class T>
        inline T GetComponent(Entity entity)
        {
            T result;
            CopyComponent(entity, ComponentType::Create<T>(), &result);
            return result;
        }

        bool UpdateComponent(Entity entity, const ComponentType& componentType, const void* source);

        template<class T>
        inline bool SetComponent(Entity entity, const T& data)
        {
            return UpdateComponent(entity, ComponentType::Create<T>(), &data);
        }
    };
} // namespace FE::ECS
