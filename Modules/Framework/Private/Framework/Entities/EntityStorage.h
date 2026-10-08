#pragma once
#include <Core/Memory/PoolAllocator.h>
#include <Framework/Entities/EntityWorld.h>
#include <festd/unordered_map.h>

namespace FE::Framework
{
    struct EntityStorage final
    {
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
        festd::vector<Archetype*> m_archetypes;
        festd::vector<ArchetypeChunk*> m_chunks;
        uint64_t m_hierarchyRevision = 0;
        uint64_t m_structureRevision = 0;
        uint64_t m_changeVersion = 1;
        uint64_t m_changeEra = 1;
        Threading::SpinLock m_versionLock;
    };


    struct ComponentLoadingState final
    {
        Rtti::TypeID m_component = Rtti::TypeID::kNull;
        uint64_t m_transition = 0;
        uint64_t m_nextTransition = 1;
    };
} // namespace FE::Framework
