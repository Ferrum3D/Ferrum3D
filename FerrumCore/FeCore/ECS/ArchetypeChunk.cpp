#include <FeCore/ECS/ArchetypeChunk.h>
#include <FeCore/ECS/EntityArchetype.h>

namespace FE::ECS
{
    void ArchetypeChunk::Init(const ArchetypeChunkDesc& desc)
    {
        FE_ASSERT_MSG(m_Data.Empty(), "Archetype chunk must be initialized only once");
        FE_ASSERT_MSG(desc.Archetype, "Archetype cannot be null");
        FE_ASSERT_MSG(desc.ByteSize > 0, "Size of archetype chunk must be greater than zero");

        m_Archetype = desc.Archetype;
        m_Data.Resize(desc.ByteSize);

        m_Capacity = m_Archetype->ChunkCapacity();

        USize beginIndex = 0;
        for (auto& type : m_Archetype->ComponentTypes())
        {
            auto nextIndex = m_Capacity * type.AlignedSize();

            ComponentStorageDesc storageDesc;
            storageDesc.Data = FE::ArraySliceMut(m_Data)(beginIndex, nextIndex);
            storageDesc.Type = type;
            m_ComponentStorages.Emplace().Init(storageDesc);

            beginIndex = nextIndex + 1;
        }
    }

    ECSResult ArchetypeChunk::AllocateEntity(UInt32& entityID)
    {
        entityID = static_cast<UInt32>(m_ComponentStorages.Front().Count());
        if (entityID == m_Capacity)
        {
            return ECSResult::AllocationError;
        }

        for (auto& storage : m_ComponentStorages)
        {
            storage.AllocateComponentUnchecked();
        }

        return ECSResult::Success;
    }

    ECSResult ArchetypeChunk::ComponentData(UInt32 entityID, const TypeID& typeID, void** componentData)
    {
        *componentData = nullptr;
        if (Full())
        {
            return ECSResult::OutOfRangeError;
        }

        for (auto& storage : m_ComponentStorages)
        {
            if (storage.CheckTypeID(typeID))
            {
                storage.ComponentData(entityID, componentData);
                return ECSResult::Success;
            }
        }

        return ECSResult::ComponentNotFoundError;
    }

    ECSResult ArchetypeChunk::DeallocateEntity(UInt32 entityID)
    {
        for (auto& storage : m_ComponentStorages)
        {
            storage.
        }
    }
} // namespace FE::ECS
