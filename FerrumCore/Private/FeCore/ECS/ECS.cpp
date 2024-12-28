#include <FeCore/ECS/ECS.h>
#include <FeCore/Memory/Memory.h>

namespace FE::ECS
{
    Archetype::Archetype(const festd::span<const uint32_t> componentSizes)
    {
        m_entitySize = 0;
        for (const uint32_t componentSize : componentSizes)
            m_entitySize += componentSize;

        m_componentSpecs.reserve(componentSizes.size());

        m_entityCount = ArchetypeChunk::CalculateEntityCount(m_entitySize);

        uint32_t chunkOffset = 0;
        for (const uint32_t componentSize : componentSizes)
        {
            ArchetypeComponentSpec componentSpec{};
            componentSpec.m_size = componentSize;
            componentSpec.m_chunkOffset = chunkOffset;

            m_componentSpecs.push_back(componentSpec);
            chunkOffset += componentSize * m_entityCount;
        }
    }


    Entity Archetype::CreateEntity(festd::vector<const ArchetypeChunk*>& globalChunkTable)
    {
        for (auto* chunk : m_chunks)
        {
            const uint32_t currentEntityCount = chunk->m_allocatedEntityCount;
            if (currentEntityCount < m_entityCount)
            {
                const uint32_t index = chunk->m_indexAllocator.AllocateIndex();
                FE_AssertDebug(index < m_entityCount);

                Entity entity;
                entity.m_chunkID = chunk->m_chunkID;
                entity.m_entityID = index;

                uint16_t* lookupTable = chunk->GetIndexLookupTable(m_entitySize, m_entityCount);
                lookupTable[index] = static_cast<uint16_t>(currentEntityCount);
                ++chunk->m_allocatedEntityCount;

                globalChunkTable.push_back(chunk);

                return entity;
            }
        }

        auto* newChunk = ArchetypeChunk::New();
        newChunk->m_archetype = this;
        newChunk->m_chunkID = globalChunkTable.size();
        globalChunkTable.push_back(newChunk);
        m_chunks.push_back(newChunk);

        const uint32_t index = newChunk->m_indexAllocator.AllocateIndex();

        Entity entity;
        entity.m_chunkID = newChunk->m_chunkID;
        entity.m_entityID = index;

        uint16_t* lookupTable = newChunk->GetIndexLookupTable(m_entitySize, newChunk->m_indexAllocator.m_freeIndices.size());
        lookupTable[index] = static_cast<uint16_t>(index);

        ++newChunk->m_allocatedEntityCount;

        return entity;
    }


    void Archetype::DestroyEntity(const Entity entity, festd::vector<const ArchetypeChunk*>& globalChunkTable)
    {
        auto chunk = const_cast<ArchetypeChunk*>(globalChunkTable.at(entity.m_chunkID));

        chunk->m_indexAllocator.FreeIndex(entity.m_entityID);
        auto* lookupTable = chunk->GetIndexLookupTable(m_entitySize, chunk->m_indexAllocator.m_freeIndices.size());
        lookupTable[entity.m_entityID] = UINT16_MAX;

        --chunk->m_allocatedEntityCount;

        if (chunk->m_allocatedEntityCount)
        {
            // DestroyEmptyChunk
        }
    }
} // namespace FE::ECS
