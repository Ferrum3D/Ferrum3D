#include <FeCore/ECS/ArchetypeChunk.h>
#include <FeCore/ECS/EntityArchetype.h>

namespace FE::ECS
{
    void ArchetypeChunk::Init(const ArchetypeChunkDesc& desc)
    {
        FE_ASSERT_MSG(m_Data.Empty(), "Archetype chunk must be initialized only once");
        FE_ASSERT_MSG(desc.Archetype, "Archetype cannot be null");
        FE_ASSERT_MSG(desc.ByteSize > 0, "Size of archetype chunk must be greater than zero");

        m_Data.Resize(desc.ByteSize);

        m_Capacity = desc.Archetype->ChunkCapacity();

        m_EntityIDs.Resize(m_Capacity);
        m_FreeList.Resize(m_Capacity);
        for (UInt16 i = 0; i < static_cast<UInt16>(m_Capacity); ++i)
        {
            m_FreeList[i] = i;
        }

        USize beginIndex = 0;
        for (auto& type : desc.Archetype->ComponentTypes())
        {
            auto nextIndex = beginIndex + m_Capacity * type.AlignedSize();

            ComponentStorageDesc storageDesc;
            storageDesc.Data = FE::ArraySliceMut(m_Data)(beginIndex, nextIndex);
            storageDesc.Type = type;
            m_ComponentStorages.Emplace().Init(storageDesc);

            beginIndex = nextIndex + 1;
        }
    }

    ECSResult ArchetypeChunk::AllocateEntity(UInt16& entityID)
    {
        auto entityIndex = static_cast<UInt16>(m_ComponentStorages.Front().Count());

        if (entityIndex == m_Capacity)
        {
            return ECSResult::OutOfMemoryError;
        }

        entityID = m_FreeList.Pop();
        if (m_EntityIndices.Size() < entityID + 1)
        {
            m_EntityIndices.Resize(entityID + 1, static_cast<UInt16>(-1));
        }

        m_EntityIndices[entityID] = entityIndex;
        m_EntityIDs[entityIndex]  = entityID;

        for (auto& storage : m_ComponentStorages)
        {
            storage.AllocateComponentUnchecked();
        }

        ++m_Version;
        return ECSResult::Success;
    }

    ECSResult ArchetypeChunk::UpdateComponent(UInt16 entityID, const TypeID& typeID, const void* source)
    {
        if (entityID >= m_EntityIndices.Size() || m_EntityIndices[entityID] == static_cast<UInt16>(-1))
        {
            return ECSResult::OutOfRangeError;
        }

        auto entityIndex = m_EntityIndices[entityID];

        for (auto& storage : m_ComponentStorages)
        {
            if (storage.CheckTypeID(typeID))
            {
                void* data;
                storage.ComponentData(entityIndex, &data);
                memcpy(data, source, storage.ElementSize());
                ++m_Version;
                return ECSResult::Success;
            }
        }

        return ECSResult::ComponentNotFoundError;
    }

    ECSResult ArchetypeChunk::CopyComponent(UInt16 entityID, const TypeID& typeID, void* destination)
    {
        if (entityID >= m_EntityIndices.Size() || m_EntityIndices[entityID] == static_cast<UInt16>(-1))
        {
            return ECSResult::OutOfRangeError;
        }

        auto entityIndex = m_EntityIndices[entityID];

        for (auto& storage : m_ComponentStorages)
        {
            if (storage.CheckTypeID(typeID))
            {
                void* data;
                storage.ComponentData(entityIndex, &data);
                memcpy(destination, data, storage.ElementSize());
                return ECSResult::Success;
            }
        }

        return ECSResult::ComponentNotFoundError;
    }

    ECSResult ArchetypeChunk::CopyComponentToChunk(UInt16 srcEntityID, UInt16 dstEntityID, const TypeID& typeID,
                                                   ArchetypeChunk* chunk)
    {
        auto srcEntityIndex = m_EntityIndices[srcEntityID];
        auto dstEntityIndex = chunk->m_EntityIndices[dstEntityID];

        void* destination = nullptr;
        for (auto& storage : chunk->m_ComponentStorages)
        {
            if (storage.CheckTypeID(typeID))
            {
                storage.ComponentData(dstEntityIndex, &destination);
                break;
            }
        }

        if (destination == nullptr)
        {
            return ECSResult::ComponentNotFoundError;
        }

        for (auto& storage : m_ComponentStorages)
        {
            if (storage.CheckTypeID(typeID))
            {
                void* data;
                storage.ComponentData(srcEntityIndex, &data);
                memcpy(destination, data, storage.ElementSize());
                return ECSResult::Success;
            }
        }

        return ECSResult::ComponentNotFoundError;
    }

    ECSResult ArchetypeChunk::DeallocateEntity(UInt16 entityID)
    {
        if (entityID >= m_EntityIndices.Size() || m_EntityIndices[entityID] == static_cast<UInt16>(-1))
        {
            return ECSResult::OutOfRangeError;
        }

        auto entityIndex          = m_EntityIndices[entityID];
        m_EntityIndices[entityID] = static_cast<UInt16>(-1);
        m_FreeList.Push(entityID);

        Int32 moveIndex = -1;
        for (auto& storage : m_ComponentStorages)
        {
            moveIndex = storage.RemoveComponent(entityIndex);
        }

        if (moveIndex != -1)
        {
            m_EntityIndices[m_EntityIDs[moveIndex]] = entityIndex;
            m_EntityIDs[entityIndex]                = m_EntityIDs[moveIndex];
        }

        ++m_Version;
        return ECSResult::Success;
    }
} // namespace FE::ECS
