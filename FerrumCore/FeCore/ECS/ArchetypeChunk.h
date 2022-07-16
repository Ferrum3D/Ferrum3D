#pragma once
#include <FeCore/ECS/ComponentStorage.h>

namespace FE::ECS
{
    class EntityArchetype;

    //! \brief Entity archetype chunk descriptor.
    struct ArchetypeChunkDesc
    {
        USize ByteSize             = 16 * 1024; //!< Size of chunk in bytes.
        EntityArchetype* Archetype = nullptr;   //!< The archetype this chunk is attached to.
    };

    //! \brief Entity archetype chunk. A chunk of memory that stores entities components of the same archetype.
    class ArchetypeChunk final
    {
        EntityArchetype* m_Archetype = nullptr;
        USize m_Capacity             = 0;
        List<Int8> m_Data;
        List<ComponentStorage> m_ComponentStorages;

    public:
        FE_STRUCT_RTTI(ArchetypeChunk, "8D700C4F-6DAE-4C92-A6D5-0B3EB53BF592");

        inline ArchetypeChunk() = default;

        void Init(const ArchetypeChunkDesc& desc);

        //! \brief Allocate entity data.
        //!
        //! \param [out] entityID - The internal ID of entity (within the chunk).
        ECSResult AllocateEntity(UInt32& entityID);

        //! \brief Deallocate entity data.
        //!
        //! \param [in] entityID - The internal ID of entity (within the chunk).
        ECSResult DeallocateEntity(UInt32 entityID);

        //! \brief Retrieve the pointer to the component to get/set its data.
        //!
        //! \param [in] entityID      - The internal ID of entity (within the chunk).
        //! \param [in] typeID        - The ID of component type.
        //! \param [in] componentData - The data of component to be copied.
        ECSResult ComponentData(UInt32 entityID, const TypeID& typeID, void** componentData);

        //! \brief Get allocated entity count.
        [[nodiscard]] inline USize Count() const
        {
            if (m_ComponentStorages.Empty())
            {
                return 0;
            }

            // Since all entities within the chunk have the same set of components, it doesn't matter what storage
            // to use to get the entity count: number of components of any type is the number of entities.
            return m_ComponentStorages.Front().Count();
        }

        [[nodiscard]] inline bool Empty() const
        {
            return m_ComponentStorages.Empty() || m_ComponentStorages.Front().Count() == 0;
        }

        [[nodiscard]] inline bool Full() const
        {
            return m_ComponentStorages.Any() && m_ComponentStorages.Front().Count() == m_Capacity;
        }

        [[nodiscard]] inline EntityArchetype* GetArchetype() const
        {
            return m_Archetype;
        }
    };
} // namespace FE::ECS
