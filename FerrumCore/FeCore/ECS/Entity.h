#pragma once
#include <FeCore/ECS/ECSCommon.h>
#include <FeCore/RTTI/RTTI.h>
#include <limits>

namespace FE::ECS
{
    //! \brief Class that represents an entity in Ferrum ECS.
    //!
    //! This class is just an index of entity, it doesn't store the components. Entity's ID is a 32 bit unsigned integer.
    //!
    //! \see EntityArchetype.
    class Entity final
    {
        EntityID m_ID;

        inline explicit Entity(EntityID id)
            : m_ID(id)
        {
        }

    public:
        FE_STRUCT_RTTI(Entity, "9D635A82-7AC3-40AB-BCA5-A0FDE06F9282");

        //! \brief Create a NULL entity that doesn't actually point to any entity.
        inline static Entity Null()
        {
            return Entity(static_cast<EntityID>(-1));
        }

        inline static Entity Create(EntityID entityID, EntityID version)
        {
            return Entity(entityID | (version << EntityConstants::EntityIDBitCount));
        }

        [[nodiscard]] inline EntityID GetID() const
        {
            return m_ID & EntityConstants::EntityIDMask;
        }

        [[nodiscard]] inline EntityID GetVersion() const
        {
            return (m_ID & EntityConstants::VersionMask) >> EntityConstants::EntityIDBitCount;
        }

        [[nodiscard]] inline bool IsNull() const
        {
            return m_ID == static_cast<EntityID>(-1);
        }

        inline explicit operator bool() const
        {
            return !IsNull();
        }
    };
} // namespace FE::ECS
