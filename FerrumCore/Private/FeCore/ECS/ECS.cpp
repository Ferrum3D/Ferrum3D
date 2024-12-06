#include "FeCore/ECS/ECS.h"
#include "FeCore/Memory/Memory.h"
#include <cstdint>

namespace FE::ECS
{
    Archetype::Archetype(festd::span<uint32_t> componentSizes)
    {
        m_entitySize = sizeof(Entity);

        m_componentSpecs.reserve(componentSizes.size());

        for (uint32_t componentSize : componentSizes)
        {
            ArchetypeComponentSpec componentSpec{};
            componentSpec.m_size = componentSize;
            componentSpec.m_chunkOffset = m_entitySize;

            m_componentSpecs.push_back(componentSpec);
            m_entitySize += componentSize;
        }

        m_chunks.reserve(1);
    }

    template<class... TComponents>
    Entity CreateEntity()
    {
        constexpr uint32_t componentSizes[] = { sizeof(TComponents)... };
        Rc<Archetype> archetype = Rc<Archetype>::DefaultNew(componentSizes);
        Entity entity = archetype->CreateEntity();
        (AddComponent<TComponents>(entity), ...);

        return entity;
    }

    Entity Archetype::CreateEntity()
    {
        for (auto* chunk : m_chunks)
        {
            const uint32_t index = chunk->m_indexAllocator.AllocateIndex();
            if (index < ArchetypeChunk::CalculateEntityCount(sizeof(this->m_entitySize)))
            {
                Entity entity{};
                entity.m_chunkID = chunk->m_chunkID;
                entity.m_entityID = index;

                auto* lookupTable = chunk->GetIndexLookupTable(m_entitySize, chunk->m_indexAllocator.m_freeIndices.size());
                lookupTable[index] = static_cast<uint16_t>(index);

                return entity;
            }
        }

        auto* newChunk = ArchetypeChunk::New();
        newChunk->m_archetype = this;
        newChunk->m_chunkID = m_chunks.size();
        m_chunks.push_back(newChunk);

        uint32_t index = newChunk->m_indexAllocator.AllocateIndex();

        Entity entity{};
        entity.m_chunkID = newChunk->m_chunkID;
        entity.m_entityID = index;

        auto* lookupTable = newChunk->GetIndexLookupTable(m_entitySize, newChunk->m_indexAllocator.m_freeIndices.size());
        lookupTable[index] = static_cast<uint16_t>(index);

        return entity;
    }

    void Archetype::DestroyEntity(Entity entity)
    {
        ArchetypeChunk* chunk = m_chunks[entity.m_chunkID];

        chunk->m_indexAllocator.FreeIndex(entity.m_entityID);
        auto* lookupTable = chunk->GetIndexLookupTable(m_entitySize, chunk->m_indexAllocator.m_freeIndices.size());
        lookupTable[entity.m_entityID] = UINT16_MAX;
    }
} // namespace FE::ECS
