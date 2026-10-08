#pragma once
#include <Framework/Entities/EntityComponentRegistry.h>

namespace FE::Framework
{
    //! @brief Canonical component layout and initialization order shared by chunks.
    struct Archetype final
    {
        //! @brief Canonical registered component metadata in column order.
        festd::vector<const EntityComponentInfo*> m_columns;
        //! @brief Aligned byte offsets of component columns.
        festd::vector<uint32_t> m_offsets;
        //! @brief Column indices sorted by initialization prerequisites.
        festd::vector<uint32_t> m_lifecycleOrder;
        //! @brief Maximum number of rows in a chunk.
        uint32_t m_capacity = 0;
        //! @brief Aligned component storage size per chunk, in bytes.
        uint32_t m_byteSize = 0;
        //! @brief Required alignment for the backing allocation.
        uint32_t m_alignment = 16;
        //! @brief Compute a canonical column layout and dependency order; invalid dependencies yield an incomplete order.
        explicit Archetype(festd::span<const EntityComponentInfo* const> columns);
        //! @brief Return the column index for a reflected type, or kInvalidIndex.
        [[nodiscard]] uint32_t Find(Rtti::TypeID type) const;
    };


    //! @brief Aligned component storage and row metadata for one archetype/residency owner.
    struct ArchetypeChunk final
    {
        //! @brief Shared canonical layout for every row in the chunk.
        Archetype& m_archetype;
        //! @brief Borrowed residency owner shared by rows in this chunk.
        EntityRegistry& m_registry;
        //! @brief Aligned backing allocation containing component values.
        std::byte* m_data;
        //! @brief Authored entity records or occupied runtime row owners.
        festd::vector<Entity*> m_entities;
        // Lifecycle metadata follows rows on compaction/migration; objects have no framework base or vtable.
        //! @brief Per-row lifecycle metadata moved with compacted values.
        festd::vector<ComponentStage> m_stages;
        //! @brief Last published change version for each component column.
        festd::vector<uint64_t> m_versions;
        //! @brief Number of occupied rows.
        uint32_t m_count = 0;

        //! @brief Allocate aligned storage for one archetype and registry.
        ArchetypeChunk(Archetype& archetype, EntityRegistry& registry);
        //! @brief Release empty storage after every occupied value has been destroyed.
        ~ArchetypeChunk();

        //! @brief Borrow storage for a valid occupied row and column.
        [[nodiscard]] void* Get(uint32_t row, uint32_t column) const;
        //! @brief Access row lifecycle metadata, which moves with the component during compaction.
        [[nodiscard]] ComponentStage& Stage(uint32_t row, uint32_t column);

        //! @brief Reserve an unconstructed row and attach the entity; the caller constructs every component.
        uint32_t Allocate(Entity& entity);
        //! @brief Relocate the last row into a destroyed row and repair the moved entity's location.
        void Free(uint32_t row);
    };
} // namespace FE::Framework
