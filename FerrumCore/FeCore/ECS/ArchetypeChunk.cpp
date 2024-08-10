#include <FeCore/ECS/ArchetypeChunk.h>
#include <FeCore/ECS/EntityArchetype.h>

namespace FE::ECS
{
    void ArchetypeChunk::Init(const ArchetypeChunkDesc& desc)
    {
        FE_ASSERT_MSG(m_Data.empty(), "Archetype chunk must be initialized only once");
        FE_ASSERT_MSG(desc.Archetype, "Archetype cannot be null");
        FE_ASSERT_MSG(desc.ByteSize > 0, "Size of archetype chunk must be greater than zero");

        m_Data.resize(desc.ByteSize);

        m_Capacity = desc.Archetype->ChunkCapacity();

        m_EntityIDs.resize(static_cast<uint32_t>(m_Capacity));
        m_FreeList.resize(static_cast<uint32_t>(m_Capacity));
        for (uint16_t i = 0; i < static_cast<uint16_t>(m_Capacity); ++i)
        {
            m_FreeList[i] = i;
        }

        uint32_t beginIndex = 0;
        for (auto& type : desc.Archetype->ComponentTypes())
        {
            const uint32_t nextIndex = beginIndex + m_Capacity * type.AlignedSize();

            ComponentStorageDesc storageDesc;
            storageDesc.Data = FE::ArraySliceMut(m_Data)(beginIndex, nextIndex);
            storageDesc.Type = type;
            m_ComponentStorages.push_back().Init(storageDesc);

            beginIndex = nextIndex;
        }
    }

    ECSResult ArchetypeChunk::AllocateEntity(uint16_t& entityID)
    {
        auto entityIndex = static_cast<uint16_t>(m_ComponentStorages.front().Count());

        if (entityIndex == m_Capacity)
        {
            return ECSResult::OutOfMemoryError;
        }

        entityID = m_FreeList.back();
        m_FreeList.pop_back();
        if (m_EntityIndices.size() < static_cast<uint32_t>(entityID + 1))
        {
            m_EntityIndices.resize(entityID + 1, static_cast<uint16_t>(-1));
        }

        m_EntityIndices[entityID] = entityIndex;
        m_EntityIDs[entityIndex] = entityID;

        for (auto& storage : m_ComponentStorages)
        {
            storage.AllocateComponentUnchecked();
        }

        ++m_Version;
        return ECSResult::Success;
    }

    ECSResult ArchetypeChunk::UpdateComponent(uint16_t entityID, const TypeID& typeID, const void* source)
    {
        if (entityID >= m_EntityIndices.size() || m_EntityIndices[entityID] == static_cast<uint16_t>(-1))
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

    ECSResult ArchetypeChunk::CopyComponent(uint16_t entityID, const TypeID& typeID, void* destination)
    {
        if (entityID >= m_EntityIndices.size() || m_EntityIndices[entityID] == static_cast<uint16_t>(-1))
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

    ECSResult ArchetypeChunk::CopyComponentToChunk(uint16_t srcEntityID, uint16_t dstEntityID, const TypeID& typeID,
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

    ECSResult ArchetypeChunk::DeallocateEntity(uint16_t entityID)
    {
        if (entityID >= m_EntityIndices.size() || m_EntityIndices[entityID] == static_cast<uint16_t>(-1))
        {
            return ECSResult::OutOfRangeError;
        }

        auto entityIndex = m_EntityIndices[entityID];
        m_EntityIndices[entityID] = static_cast<uint16_t>(-1);
        m_FreeList.push_back(entityID);

        int32_t moveIndex = -1;
        for (auto& storage : m_ComponentStorages)
        {
            moveIndex = storage.RemoveComponent(entityIndex);
        }

        if (moveIndex != -1)
        {
            m_EntityIndices[m_EntityIDs[moveIndex]] = entityIndex;
            m_EntityIDs[entityIndex] = m_EntityIDs[moveIndex];
        }

        ++m_Version;
        return ECSResult::Success;
    }
} // namespace FE::ECS
