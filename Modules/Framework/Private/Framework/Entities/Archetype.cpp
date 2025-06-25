#include <FeCore/Memory/FiberTempAllocator.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Modules/Environment.h>
#include <Framework/Entities/Archetype.h>
#include <Framework/Entities/EntityComponentRegistry.h>
#include <festd/bit_vector.h>

namespace FE::Framework
{
    namespace
    {
        constexpr uint32_t kBitsPerWord = sizeof(uint64_t) * 8;

        bool IsChunkSizeValid(const uint32_t byteSize, const uint32_t entityByteSize, const uint32_t entityCount)
        {
            const uint32_t bitsetSize = Math::CeilDivide(entityCount, kBitsPerWord);
            const uint32_t requiredByteSize = (entityByteSize + sizeof(uint16_t)) * entityCount + bitsetSize * sizeof(uint64_t);
            return requiredByteSize <= byteSize;
        }


        Memory::Pool<Archetype> GArchetypePool{ "EntityArchetypePool" };
        Memory::Pool<ArchetypeChunk> GArchetypeChunkPool{ "EntityArchetypeChunkPool" };
    } // namespace


    Archetype* Archetype::Create(EntityRegistry* registry, festd::span<const ComponentTypeID> componentTypes)
    {
        return GArchetypePool.New(registry, componentTypes);
    }


    void Archetype::Destroy(const Archetype* archetype)
    {
        GArchetypePool.Delete(archetype);
    }


    EntityAllocationResult Archetype::AllocateEntity()
    {
        for (ArchetypeChunk* chunk : m_chunks)
        {
            const uint32_t entityIndex = chunk->Allocate();
            if (entityIndex != kInvalidIndex)
            {
                EntityAllocationResult result;
                result.m_chunk = chunk;
                result.m_entityIndex = entityIndex;
                return result;
            }
        }

        auto* newChunk = ArchetypeChunk::Create();
        newChunk->Setup(m_chunks.size(), this, m_chunkByteSize);
        m_chunkByteSize *= 2;
        m_chunks.push_back(newChunk);

        EntityAllocationResult result;
        result.m_chunk = newChunk;
        result.m_entityIndex = newChunk->Allocate();
        return result;
    }


    Archetype::Archetype(EntityRegistry* registry, const festd::span<const ComponentTypeID> componentTypes)
        : m_registry(registry)
    {
        m_componentTypes.reserve(componentTypes.size());
        m_componentTypeIDs.reserve(componentTypes.size());
        m_components.reserve(m_componentTypes.size());

        const auto& componentRegistry = EntityComponentRegistry::Get();
        for (const ComponentTypeID typeID : componentTypes)
        {
            const EntityComponentInfo* info = componentRegistry.GetComponentInfo(typeID);
            m_componentTypes.push_back(info);
        }

        festd::sort(m_componentTypes, [](const EntityComponentInfo* lhs, const EntityComponentInfo* rhs) {
            if (lhs->m_byteSize == rhs->m_byteSize)
                return lhs->m_typeID.m_value < rhs->m_typeID.m_value;
            return lhs->m_byteSize > rhs->m_byteSize;
        });

        m_entityByteSize = 0;
        for (const EntityComponentInfo* info : m_componentTypes)
        {
            m_entityByteSize = AlignUp(m_entityByteSize, info->m_byteAlignment);

            ArchetypeComponentDesc& desc = m_components.push_back();
            desc.m_byteSize = info->m_byteSize;
            desc.m_byteOffset = m_entityByteSize;

            m_entityByteSize += desc.m_byteSize;
            m_componentTypeIDs.push_back(info->m_typeID);
        }
    }


    Archetype::~Archetype()
    {
        for (const ArchetypeChunk* chunk : m_chunks)
            GArchetypeChunkPool.Delete(chunk);
    }


    bool Archetype::MatchesAll(const festd::span<const ComponentTypeID> includedComponentTypes) const
    {
        for (const ComponentTypeID typeID : includedComponentTypes)
        {
            const auto it = festd::find(m_componentTypeIDs, typeID);
            if (it == m_componentTypeIDs.end())
                return false;
        }

        return true;
    }


    bool Archetype::MatchesAny(festd::span<const ComponentTypeID> includedComponentTypes) const
    {
        for (const ComponentTypeID typeID : includedComponentTypes)
        {
            const auto it = festd::find(m_componentTypeIDs, typeID);
            if (it != m_componentTypeIDs.end())
                return true;
        }

        return false;
    }


    ArchetypeChunk* ArchetypeChunk::Create()
    {
        return GArchetypeChunkPool.New();
    }


    void ArchetypeChunk::Setup(const uint32_t chunkID, Archetype* archetype, const uint32_t byteSize)
    {
        m_archetype = archetype;
        m_chunkID = chunkID;
        m_byteSize = byteSize;

        auto* allocator = Env::GetStaticAllocator(Memory::StaticAllocatorType::kDefault);
        m_data = static_cast<std::byte*>(allocator->allocate(byteSize));

        const uint32_t bytesPerEntity = archetype->m_entityByteSize + sizeof(uint16_t);
        const uint32_t bitsPerEntity = bytesPerEntity * 8 + 1;
        m_entityCount = byteSize * 8 / bitsPerEntity;

        while (!IsChunkSizeValid(byteSize, archetype->m_entityByteSize, m_entityCount))
        {
            FE_AssertDebug(m_entityCount > 1);
            --m_entityCount;
        }

        const uint32_t indexLookupTableSize = m_entityCount * sizeof(uint16_t);
        const uint32_t bitsetSize = Math::CeilDivide(m_entityCount, kBitsPerWord) * sizeof(uint64_t);
        const uint32_t componentDataSize = archetype->m_entityByteSize * m_entityCount;

        m_indexLookupTable = reinterpret_cast<uint16_t*>(m_data + componentDataSize);
        m_allocatedEntitiesBitSet = reinterpret_cast<uint64_t*>(m_data + componentDataSize + indexLookupTableSize);
        FE_Assert(m_byteSize >= componentDataSize + indexLookupTableSize + bitsetSize);
    }


    uint32_t ArchetypeChunk::Allocate() const
    {
        const uint32_t wordCount = Math::CeilDivide(m_entityCount, kBitsPerWord);
        for (uint32_t wordIndex = 0; wordIndex < wordCount; ++wordIndex)
        {
            const uint64_t currentWord = ~m_allocatedEntitiesBitSet[wordIndex];
            if (uint32_t bitIndex; Bit::ScanForward(bitIndex, currentWord))
            {
                const uint32_t result = wordIndex * kBitsPerWord + bitIndex;
                if (result >= m_entityCount)
                    return kInvalidIndex;

                m_allocatedEntitiesBitSet[wordIndex] |= UINT64_C(1) << bitIndex;
                return result;
            }
        }

        return kInvalidIndex;
    }


    void ArchetypeChunk::Free(const uint32_t entityIndex) const
    {
        const uint32_t wordIndex = entityIndex / kBitsPerWord;
        const uint32_t bitIndex = entityIndex % kBitsPerWord;

        FE_AssertDebug(m_allocatedEntitiesBitSet[wordIndex] & (UINT64_C(1) << bitIndex));
        m_allocatedEntitiesBitSet[wordIndex] &= ~(UINT64_C(1) << bitIndex);
    }
} // namespace FE::Framework
