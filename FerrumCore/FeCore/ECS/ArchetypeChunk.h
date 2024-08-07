﻿#pragma once
#include <FeCore/ECS/ComponentStorage.h>
#include <functional>

namespace FE::ECS
{
    class EntityArchetype;
    class ArchetypeChunk;

    //! \brief Entity archetype chunk descriptor.
    struct ArchetypeChunkDesc
    {
        uint32_t ByteSize = 16 * 1024;        //!< Size of chunk in bytes.
        EntityArchetype* Archetype = nullptr; //!< The archetype this chunk is attached to.
    };

    //! \brief Entity archetype chunk. A chunk of memory that stores entities with components of the same archetype.
    class ArchetypeChunk final
    {
        uint32_t m_Capacity = 0;
        eastl::vector<Int8> m_Data;
        eastl::vector<ComponentStorage> m_ComponentStorages;

        eastl::vector<UInt16> m_EntityIndices;
        eastl::vector<UInt16> m_EntityIDs;

        eastl::vector<UInt16> m_FreeList;

        UInt32 m_Version = 0;

        inline ArchetypeChunk() = default;

    public:
        FE_STRUCT_RTTI(ArchetypeChunk, "8D700C4F-6DAE-4C92-A6D5-0B3EB53BF592");

        // !\brief allocate an Archetype chunk, doesn't call Init().
        inline static ArchetypeChunk* Allocate()
        {
            void* ptr = Memory::DefaultAllocate(sizeof(ArchetypeChunk), alignof(ArchetypeChunk));
            return new (ptr) ArchetypeChunk;
        }

        //! \brief Deallocate the chunk, the instance is no longer valid after the call.
        inline void Deallocate()
        {
            Memory::DefaultDelete(this);
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

        inline ComponentStorage* GetComponentStorage(const TypeID& typeID)
        {
            for (auto& storage : m_ComponentStorages)
            {
                if (storage.CheckTypeID(typeID))
                {
                    return &storage;
                }
            }

            return nullptr;
        }

        //! \brief Get allocated entity count.
        [[nodiscard]] inline uint32_t Count() const
        {
            if (m_ComponentStorages.empty())
            {
                return 0;
            }

            // Since all entities within the chunk have the same set of components, it doesn't matter what storage
            // to use to get the entity count: number of components of any type is the number of entities.
            return m_ComponentStorages.front().Count();
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
            return m_ComponentStorages.empty() || m_ComponentStorages.front().Count() == 0;
        }

        //! \brief Check if the chunk is full of entities.
        [[nodiscard]] inline bool Full() const
        {
            return !m_ComponentStorages.empty() && m_ComponentStorages.front().Count() == m_Capacity;
        }
    };
} // namespace FE::ECS
