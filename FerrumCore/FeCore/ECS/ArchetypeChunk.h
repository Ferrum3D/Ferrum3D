#pragma once
#include <FeCore/ECS/ComponentStorage.h>

namespace FE::ECS
{
    class EntityArchetype;
    class ArchetypeChunk;

    //! \brief Entity archetype chunk descriptor.
    struct ArchetypeChunkDesc
    {
        USize ByteSize             = 16 * 1024; //!< Size of chunk in bytes.
        EntityArchetype* Archetype = nullptr;   //!< The archetype this chunk is attached to.
    };

    //! \brief Entity archetype chunk. A chunk of memory that stores entities with components of the same archetype.
    class ArchetypeChunk final
    {
        USize m_Capacity = 0;
        List<Int8> m_Data;
        List<ComponentStorage> m_ComponentStorages;

        List<UInt16> m_EntityIndices;

        UInt32 m_Version = 0;

        UInt16 m_EntityID = 0;

        inline ArchetypeChunk() = default;

        inline UInt16 GetEntityID()
        {
            return m_EntityID++;
        }

    public:
        FE_STRUCT_RTTI(ArchetypeChunk, "8D700C4F-6DAE-4C92-A6D5-0B3EB53BF592");

        // !\brief allocate an Archetype chunk, doesn't call Init().
        inline static ArchetypeChunk* Allocate()
        {
            FE_STATIC_SRCPOS(position);
            void* ptr = GlobalAllocator<HeapAllocator>::Get().Allocate(sizeof(ArchetypeChunk), alignof(ArchetypeChunk), position);
            return new (ptr) ArchetypeChunk;
        }

        //! \brief Deallocate the chunk, the instance is no longer valid after the call.
        inline void Deallocate()
        {
            this->~ArchetypeChunk();
            FE_STATIC_SRCPOS(position);
            GlobalAllocator<HeapAllocator>::Get().Deallocate(this, position, sizeof(ArchetypeChunk));
        }

        void Init(const ArchetypeChunkDesc& desc);

        //! \brief Allocate entity data.
        //!
        //! \param [out] entityID - The internal ID of entity (within the chunk).
        ECSResult AllocateEntity(UInt16& entityID);

        //! \brief Deallocate entity data.
        //!
        //! \param [in] entityID - The internal ID of entity (within the chunk).
        ECSResult DeallocateEntity(UInt16 entityID);

        //! \brief Set component data.
        //!
        //! \param [in] entityID - The internal ID of entity (within the chunk).
        //! \param [in] typeID   - The ID of component type.
        //! \param [in] source   - The data of component to be copied.
        ECSResult UpdateComponent(UInt16 entityID, const TypeID& typeID, const void* source);

        //! \brief Get component data.
        //!
        //! \param [in] entityID    - The internal ID of entity (within the chunk).
        //! \param [in] typeID      - The ID of component type.
        //! \param [in] destination - The buffer to copy the component data to.
        ECSResult CopyComponent(UInt16 entityID, const TypeID& typeID, void* destination);

        //! \brief Copy component data to another chunk.
        //!
        //! \param [in] srcEntityID - The internal ID of entity (within the chunk).
        //! \param [in] dstEntityID - The internal ID of entity (within the chunk).
        //! \param [in] typeID      - The ID of component type.
        //! \param [in] destination - The buffer to copy the component data to.
        ECSResult CopyComponentToChunk(UInt16 srcEntityID, UInt16 dstEntityID, const TypeID& typeID, ArchetypeChunk* chunk);

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

        //! \brief Get the version of the chunk - a number that gets incremented after every change.
        [[nodiscard]] inline UInt32 Version() const
        {
            return m_Version;
        }

        //! \brief Check if the chunk has been changed after the specified version. Same as `version != chunk.Version()`
        [[nodiscard]] inline bool DidChange(UInt32 version) const
        {
            return m_Version != version;
        }

        //! \brief Check if the chunk is doesn't store any entities.
        [[nodiscard]] inline bool Empty() const
        {
            return m_ComponentStorages.Empty() || m_ComponentStorages.Front().Count() == 0;
        }

        //! \brief Check if the chunk is full of entities.
        [[nodiscard]] inline bool Full() const
        {
            return m_ComponentStorages.Any() && m_ComponentStorages.Front().Count() == m_Capacity;
        }
    };
} // namespace FE::ECS
