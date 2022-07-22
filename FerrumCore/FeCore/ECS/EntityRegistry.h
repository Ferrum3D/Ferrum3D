#pragma once
#include <FeCore/Containers/SparseSet.h>
#include <FeCore/ECS/Entity.h>
#include <FeCore/ECS/EntityArchetype.h>
#include <FeCore/Memory/Object.h>

namespace FE::ECS
{
    class EntityQuery;

    class EntityRegistry final : public Object<IObject>
    {
        struct EntityData
        {
            ArchetypeChunk* Chunk = nullptr;
            Int16 ArchetypeID     = -1;
            UInt16 EntityID       = 0;

            [[nodiscard]] inline bool IsEmpty() const
            {
                return Chunk == nullptr;
            }
        };

        SparseSet<EntityID, EntityData> m_Data;
        SparseSet<EntityID, EntityID> m_VersionTable;
        List<EntityArchetype> m_Archetypes;

        EntityID m_EntityID = 0;
        inline EntityID GetEntityID()
        {
            return m_EntityID++;
        }

        void MoveToArchetype(Entity entity, Int16 archetypeIndex);

    public:
        FE_CLASS_RTTI(EntityRegistry, "1A7583F3-7C99-4CDF-9C41-27B303C82134");

        inline EntityRegistry()           = default;
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

        inline UInt32 GetComponentCount(Entity entity)
        {
            return static_cast<UInt32>(GetComponentTypes(entity).Length());
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
