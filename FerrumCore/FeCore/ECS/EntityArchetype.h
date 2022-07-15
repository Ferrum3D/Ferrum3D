#pragma once
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/ECS/ComponentType.h>
#include <FeCore/Memory/SharedPtr.h>

namespace FE::ECS
{
    //! \brief Describes an entity archetype - a collection of component types and associated component data pools.
    //!
    //! Entity archetype is a unique combination of component types (see \ref ComponentType). A component is contained
    //! within a particular archetype if and only if it has exactly the same set of components. For example, if we
    //! have these entities:
    //!
    //! ```
    //! Entity0 { Translation, Rotation, Renderer, Physics }
    //! Entity1 { Translation, Rotation, Renderer }
    //! Entity2 { Translation, Rotation, Renderer }
    //! Entity3 { Translation, Renderer, Physics }
    //! ```
    //!
    //! Only Entity1 and Entity2 will have the same archetype. Entity0 and Entity3 will have dedicated archetypes.
    //! But all this entities have some components in common, so we can get all of them from a system using entity queries.
    //! For example here is visualization of call `Query<Translation, Rotation>()`:
    //!
    //! ```
    //!     Entity0            Entity1       Entity2            Entity3
    //!        |                  |             |                  |
    //!        v                  v             v                  v
    //! +-------------+    +-------------+-------------+    +-------------+
    //! | Translation | <<<| Translation | Translation | <<<| Translation | <<<
    //! |  Rotation   | <<<|  Rotation   |  Rotation   | <<<|             | <xx
    //! |  Renderer   |    |  Renderer   |  Renderer   |    |  Renderer   |
    //! |   Physics   |    |             |             |    |   Physics   |
    //! +-------------+    +-------------+-------------+    +-------------+
    //!   Archetype0                Archetype1                Archetype2
    //! ```
    //!
    //! Here `<xx` shows that the Rotation component was not assigned to the Entity3. That's why the query will return only
    //! these entities: Entity0, Entity1, Entity2.
    //!
    //! The archetypes are built automatically by the Entity Component System when an entity with previously unknown
    //! archetype is created.
    class EntityArchetype : public Object<IObject>
    {
        friend class EntityArchetypeBuilder;

        List<ComponentType> m_Layout;

    public:
        FE_CLASS_RTTI(EntityArchetype, "EC825348-557C-4728-A44B-9CB7FCADA938");

        inline EntityArchetype() = default;

        //! \brief Create a new entity archetype from a list of component types.
        EntityArchetype(const ArraySlice<ComponentType>& layout); // NOLINT(google-explicit-constructor)
        inline ~EntityArchetype() override = default;
    };

    //! \brief This class helps building instances of EntityArchetype.
    //!
    //! Example usage:
    //! ```cpp
    //! auto archetype = EntityArchetypeBuilder{}
    //!     .AddComponent<TranslationComponent>()
    //!     .AddComponent<RotationComponent>()
    //!     .Build();
    //! ```
    class EntityArchetypeBuilder
    {
        Shared<EntityArchetype> m_Archetype;

    public:
        inline EntityArchetypeBuilder()
        {
            m_Archetype = MakeShared<EntityArchetype>();
        }

        //! \brief Add a component type to EntityArchetype.
        inline void AddComponentType(const ComponentType& componentType)
        {
            m_Archetype->m_Layout.Push(componentType);
        }

        //! \brief Add a component type to EntityArchetype.
        template<class T>
        inline void AddComponentType()
        {
            m_Archetype->m_Layout.Push(ComponentType::Create<T>());
        }

        //! \brief Build entity archetype.
        inline Shared<EntityArchetype> Build()
        {
            return m_Archetype;
        }
    };
} // namespace FE::ECS
