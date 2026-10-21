#include <Framework/Entities/Archetype.h>
#include <Framework/Entities/Entity.h>

namespace FE::Framework
{
    Archetype::Archetype(const festd::span<const EntityComponentInfo* const> columns)
    {
        m_columns.assign(columns.begin(), columns.end());

        uint32_t rowSize = 0;
        for (const auto* column : columns)
        {
            rowSize += column->m_type->m_size + column->m_type->m_alignment - 1;
            m_alignment = Math::Max(m_alignment, column->m_type->m_alignment);
        }

        m_byteSize = Math::Max(16u * 1024, AlignUp(rowSize, m_alignment));
        m_capacity = columns.empty() ? 1024 : Math::Max(1u, m_byteSize / rowSize);
        m_offsets.resize(columns.size());

        for (;; --m_capacity)
        {
            uint32_t offset = 0;
            for (uint32_t i = 0; i < columns.size(); ++i)
            {
                offset = AlignUp(offset, columns[i]->m_type->m_alignment);
                m_offsets[i] = offset;
                offset += columns[i]->m_type->m_size * m_capacity;
            }

            if (offset <= m_byteSize)
                break;
        }
    }


    uint32_t Archetype::Find(const Rtti::TypeID type) const
    {
        for (uint32_t i = 0; i < m_columns.size(); ++i)
        {
            if (m_columns[i]->m_type->m_id == type)
                return i;
        }

        return kInvalidIndex;
    }


    ArchetypeChunk::ArchetypeChunk(Archetype& archetype, EntityRegistry& registry)
        : m_archetype(archetype)
        , m_registry(registry)
    {
        m_data = static_cast<std::byte*>(Memory::DefaultAllocate(archetype.m_byteSize, archetype.m_alignment));
        m_entities.resize(archetype.m_capacity, nullptr);
        m_versions.resize(archetype.m_columns.size(), 0);
        m_stages.resize(archetype.m_capacity * archetype.m_columns.size(), ComponentStage::kNone);
        m_addedVersions.resize(archetype.m_capacity * archetype.m_columns.size(), 0);
    }


    ArchetypeChunk::~ArchetypeChunk()
    {
        FE_AssertDebug(m_count == 0);
        Memory::DefaultFree(m_data);
    }


    void* ArchetypeChunk::Get(const uint32_t row, const uint32_t column) const
    {
        return m_data + m_archetype.m_offsets[column] + row * m_archetype.m_columns[column]->m_type->m_size;
    }


    ComponentStage& ArchetypeChunk::Stage(const uint32_t row, const uint32_t column)
    {
        return m_stages[column * m_archetype.m_capacity + row];
    }


    uint64_t& ArchetypeChunk::AddedVersion(const uint32_t row, const uint32_t column)
    {
        return m_addedVersions[column * m_archetype.m_capacity + row];
    }


    uint32_t ArchetypeChunk::Allocate(Entity& entity)
    {
        FE_AssertDebug(m_count < m_archetype.m_capacity);

        const uint32_t row = m_count++;
        m_entities[row] = &entity;
        for (uint32_t column = 0; column < m_archetype.m_columns.size(); ++column)
        {
            Stage(row, column) = ComponentStage::kNone;
            AddedVersion(row, column) = 0;
        }

        return row;
    }


    void ArchetypeChunk::Free(const uint32_t row)
    {
        FE_AssertDebug(row < m_count);

        const uint32_t last = --m_count;
        if (row != last)
        {
            for (uint32_t i = 0; i < m_archetype.m_columns.size(); ++i)
            {
                const auto& type = *m_archetype.m_columns[i]->m_type;
                type.m_moveConstructor(Get(row, i), Get(last, i));
                type.m_destructor(Get(last, i));
                Stage(row, i) = Stage(last, i);
                AddedVersion(row, i) = AddedVersion(last, i);
            }

            m_entities[row] = m_entities[last];
            m_entities[row]->m_row = row;
        }

        m_entities[last] = nullptr;
    }
} // namespace FE::Framework
