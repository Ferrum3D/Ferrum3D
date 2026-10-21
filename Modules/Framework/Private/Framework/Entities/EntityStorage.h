#pragma once
#include <Core/Memory/PoolAllocator.h>
#include <Framework/Entities/EntityRegistryInternal.h>
#include <Framework/Entities/EntityWorld.h>
#include <festd/unordered_map.h>

namespace FE::Framework
{
    struct EntityStorage final
    {
        ArchetypeChunk& GetAvailableChunk(Archetype& archetype, EntityRegistry& registry);
        void RowAllocated(ArchetypeChunk& chunk);
        void FreeRow(ArchetypeChunk& chunk, uint32_t row);

        struct Slot
        {
            Entity* m_entity = nullptr;
            uint32_t m_generation = 1;
        };

        Memory::Pool<Entity> m_entities{ "Entity/WorldPool" };
        festd::vector<Slot> m_slots;
        festd::vector<uint32_t> m_freeSlots;
        festd::unordered_dense_map<Uuid, Entity*> m_uuidLookup;
        festd::vector<EntityRegistry*> m_registries;
        Internal::RegistryIDPool m_registryIds;
        festd::vector<Archetype*> m_archetypes;
        festd::vector<ArchetypeChunk*> m_chunks;
        uint64_t m_hierarchyRevision = 0;
        uint64_t m_structureRevision = 0;
        uint64_t m_changeVersion = 1;
        uint64_t m_changeEra = 1;
        uint64_t m_additionVersion = 0;
        Threading::SpinLock m_versionLock;

    private:
        struct ChunkKey
        {
            Archetype* m_archetype;
            EntityRegistry* m_registry;

            bool operator==(const ChunkKey&) const = default;
        };


        struct ChunkKeyHash
        {
            size_t operator()(const ChunkKey& key) const
            {
                return HashAll(key.m_archetype, key.m_registry);
            }
        };

        festd::unordered_dense_map<ChunkKey, festd::inline_vector<ArchetypeChunk*>, ChunkKeyHash> m_availableChunks;
    };
} // namespace FE::Framework
