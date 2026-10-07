#pragma once
#include <Framework/Entities/EntityComponentRegistry.h>

namespace FE::Framework
{
    struct Archetype final
    {
        festd::vector<const EntityComponentInfo*> m_columns;
        festd::vector<uint32_t> m_offsets;
        festd::vector<uint32_t> m_lifecycleOrder;
        uint32_t m_capacity = 0;
        uint32_t m_byteSize = 0;
        uint32_t m_alignment = 16;
        explicit Archetype(festd::span<const EntityComponentInfo* const> columns);
        [[nodiscard]] uint32_t Find(Rtti::TypeID type) const;
    };


    struct ArchetypeChunk final
    {
        Archetype& m_archetype;
        EntityRegistry& m_registry;
        std::byte* m_data;
        festd::vector<Entity*> m_entities;
        // Lifecycle metadata follows rows on compaction/migration; objects have no framework base or vtable.
        festd::vector<uint8_t> m_stages;
        festd::vector<uint64_t> m_versions;
        uint32_t m_count = 0;
        ArchetypeChunk(Archetype& archetype, EntityRegistry& registry);
        ~ArchetypeChunk();
        [[nodiscard]] void* Get(uint32_t row, uint32_t column) const;
        [[nodiscard]] uint8_t& Stage(uint32_t row, uint32_t column);
        uint32_t Allocate(Entity& entity);
        // Caller has destroyed the removed row. Relocates the last occupied row and repairs its Entity location.
        void Free(uint32_t row);
    };
} // namespace FE::Framework
