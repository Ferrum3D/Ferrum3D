#pragma once
#include <FeCore/ECS/Entity.h>
#include <FeCore/ECS/EntityArchetype.h>
#include <FeCore/ECS/EntityRegistry.h>

namespace FE::ECS
{
    class EntityQueryBuilder
    {
        friend class EntityQuery;

        EntityArchetype* m_ComponentTypes;
        EntityQuery* m_Query;

        inline EntityQueryBuilder(EntityArchetype* componentTypes, EntityQuery* query)
            : m_ComponentTypes(componentTypes)
            , m_Query(query)
        {
        }

    public:
        //! \brief Add a component type to EntityQuery.
        inline void AddComponentType(const ComponentType& componentType)
        {
            m_ComponentTypes->m_Layout.Push(componentType);
        }

        //! \brief Add a component type to EntityQuery.
        template<class T>
        inline void AddComponentType()
        {
            m_ComponentTypes->m_Layout.Push(ComponentType::Create<T>());
        }

        //! \brief Build entity archetype.
        inline EntityQuery& Build()
        {
            return *m_Query;
        }
    };

    //! \brief Entity query allows to iterate over a group of selected entities and their components.
    class EntityQuery final
    {
        friend class EntityRegistry;

        EntityRegistry* m_Registry;
        List<EntityArchetype*> m_Archetypes;

        EntityArchetype m_IncludeNone;
        EntityArchetype m_IncludeAll;
        EntityArchetype m_IncludeAny;

    public:
        FE_STRUCT_RTTI(EntityQuery, "BB78DEF5-2D28-4E85-9E78-3CD1E7263B45");

        inline explicit EntityQuery(EntityRegistry* registry)
            : m_Registry(registry)
        {
        }

        inline EntityQueryBuilder NoneOf()
        {
            return { &m_IncludeNone, this };
        }

        inline EntityQueryBuilder AllOf()
        {
            return { &m_IncludeAll, this };
        }

        inline EntityQueryBuilder AnyOf()
        {
            return { &m_IncludeAny, this };
        }

        inline void Update()
        {
            m_Registry->UpdateEntityQuery(this);
        }
    };
} // namespace FE::ECS
