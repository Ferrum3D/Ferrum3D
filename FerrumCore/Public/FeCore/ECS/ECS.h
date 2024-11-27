#pragma once
#include <FeCore/Utils/UUID.h>

namespace FE::ECS
{
    struct Entity
    {
        UUID Id;

        Entity()
            : Id(UUID{})
        {
        }
    };

    struct ArchetypeChunk;

    struct Archetype
    {
        festd::vector<ArchetypeChunk*> m_chunks;
    };

    struct ArchetypeChunk
    {
        festd::vector<bool> freeEntities;
        uint8_t* data;
        UUID* entities;
        size_t byteSize;
        size_t entityCount;

        ArchetypeChunk(size_t capacity, size_t componentSize)
        {
            byteSize = capacity * componentSize;
            data = new uint8_t[byteSize];
            entities = new UUID[capacity];
            entityCount = 0;
        }

        ~ArchetypeChunk()
        {
            delete[] data;
            delete[] entities;
        }

        inline void* getComponentPtr(size_t index, size_t componentOffset, size_t componentSize)
        {
            return data + componentOffset + index * componentSize;
        }

        size_t addChunkEntity();
        size_t removeChunkEntity(size_t index);
    };

    Entity createEntity(Archetype& archetype);
    void deleteEntity(Archetype& archetype, UUID entity);

} // namespace FE::ECS
