#include "FeCore/ECS/ECS.h"
#include "FeCore/Memory/Memory.h"
#include <FeCore/Utils/UUID.h>
#include <cstdint>

namespace FE::ECS
{
    size_t ArchetypeChunk::addChunkEntity()
    {
        for (eastl_size_t i = 0; i < freeEntities.size(); ++i)
        {
            if (!freeEntities[i])
            {
                freeEntities[i] = true;
                entityCount++;
                return i;
            }
        }

        return _CRT_SIZE_MAX;
    }

    size_t ArchetypeChunk::removeChunkEntity(size_t index)
    {
        eastl_size_t sIndex = static_cast<eastl_size_t>(index);

        if (sIndex >= freeEntities.size() || !freeEntities[sIndex])
        {
            return _CRT_SIZE_MAX;
        }

        size_t componentSize = (byteSize > 0 && freeEntities.size() > 0) ? byteSize / freeEntities.size() : 0;

        if (index != entityCount - 1)
        {
            std::memcpy(data + index * componentSize, data + (entityCount - 1) * componentSize, componentSize);
            std::memcpy(entities + index, entities + (entityCount - 1), componentSize);
        }

        freeEntities[sIndex] = false;

        entityCount--;
        return index;
    }

    Entity ECS::createEntity(Archetype& archetype)
    {
        ArchetypeChunk* targetChunk = nullptr;

        for (auto* chunk : archetype.m_chunks)
        {
            if (chunk->entityCount < chunk->freeEntities.size())
            {
                targetChunk = chunk;
                break;
            }
        }

        if (!targetChunk)
        {
            eastl_size_t capacity = 128;
            eastl_size_t componentSize = 64;
            targetChunk = new ArchetypeChunk(capacity, componentSize);

            targetChunk->freeEntities.resize(capacity, false);
            archetype.m_chunks.push_back(targetChunk);
        }

        size_t index = targetChunk->addChunkEntity();
        if (index == _CRT_SIZE_MAX)
        {
            return {};
        }

        Entity newEntity;
        targetChunk->entities[index] = newEntity.Id;

        return newEntity;
    }


    void ECS::deleteEntity(Archetype& archetype, UUID entityId)
    {
        for (auto* chunk : archetype.m_chunks)
        {
            for (size_t i = 0; i < chunk->entityCount; ++i)
            {
                if (chunk->entities[i] == entityId)
                {
                    chunk->removeChunkEntity(i);
                    return;
                }
            }
        }
    }
} // namespace FE::ECS
