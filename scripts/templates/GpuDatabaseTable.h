#pragma once
#include <Graphics/Database/Database.h>
#include <Graphics/Tables/Forwards.h>
{% for include in table.includes['cpp'] %}
#include {{ include }}
{% endfor %}
namespace FE::Graphics
{
    struct {{ table.name }} final : public DB::TableBase
    {
        FE_RTTI("{{ table.id }}");

        static constexpr uint32_t kRowsPerPage = {{ table.element_count_per_page }};
        {% for name, offset in table.offsets.items() %}
        static constexpr uint32_t kOffset_{{ name }} = {{ offset }};
        {%- endfor %}

        static_assert(kRowsPerPage > 0);

        struct Row final
        {
            {%- for column in table.columns %}
            DB::ElementHandle<{{ column.typename }}, kOffset_{{ column.name }}> {{ column.name }};
            {%- endfor %}
        };

        struct RWRow final
        {
            {%- for column in table.columns %}
            DB::RWElementHandle<{{ column.typename }}, kOffset_{{ column.name }}> {{ column.name }};
            {%- endfor %}
        };

        using Instance = BufferPointer;

        {{ table.name }}(DB::Database* database)
            : TableBase(database, kRowsPerPage)
        {
        }

        [[nodiscard]] DB::Ref<{{ table.name }}> AllocateRow()
        {
            const uint32_t rowIndex = AllocateRowUninitialized();
            const RWRow row = WriteRow(rowIndex);
            {%- for column in table.columns %}
            row.{{ column.name }}.Construct();
            {%- endfor %}
            return DB::Ref<{{ table.name }}>{ rowIndex };
        }

        [[nodiscard]] DB::Slice<{{ table.name }}> AllocateRows(const uint32_t rowCount)
        {
            const RowRangeHandle range = AllocateRowsUninitialized(rowCount);
            FE_AssertDebug(range.m_size == rowCount);
            for (uint32_t rowIndex = 0; rowIndex < rowCount; ++rowIndex)
            {
                const RWRow row = WriteRow(rowIndex + range.m_offset);
                {%- for column in table.columns %}
                row.{{ column.name }}.Construct();
                {%- endfor %}
            }

            return DB::Slice<{{ table.name }}>{ range.m_offset, range.m_size };
        }

        void Free(const DB::Ref<{{ table.name }}> row)
        {
            TableBase::Free(row.m_rowIndex);
        }

        void Free(const DB::Slice<{{ table.name }}> rows)
        {
            TableBase::Free(RowRangeHandle{ rows.m_rowIndex, rows.m_count });
        }

        [[nodiscard]] Row ReadRow(const uint32_t rowIndex)
        {
            const uint32_t pageIndex = rowIndex / kRowsPerPage;
            const uint32_t localRowIndex = rowIndex % kRowsPerPage;

            const DB::StoragePage* page = m_pages[pageIndex];
            const std::byte* storage = page->GetHostStorage();

            Row row;
            {%- for column in table.columns %}
            row.{{ column.name }}.Setup(storage, localRowIndex);
            {%- endfor %}
            return row;
        }

        [[nodiscard]] Row ReadRow(const DB::Ref<{{ table.name }}> reference)
        {
            return ReadRow(reference.m_rowIndex);
        }

        [[nodiscard]] RWRow WriteRow(const uint32_t rowIndex)
        {
            const uint32_t pageIndex = rowIndex / kRowsPerPage;
            const uint32_t localRowIndex = rowIndex % kRowsPerPage;

            DB::StoragePage* page = m_pages[pageIndex];
            m_database->MarkPageDirty(page);
            std::byte* storage = page->GetHostStorage();

            RWRow row;
            {%- for column in table.columns %}
            row.{{ column.name }}.Setup(storage, localRowIndex);
            {%- endfor %}
            return row;
        }

        [[nodiscard]] RWRow WriteRow(const DB::Ref<{{ table.name }}> reference)
        {
            return WriteRow(reference.m_rowIndex);
        }
        {% for column in table.columns %}
        void CopyColumn(const DB::Slice<{{ table.name }}> destination, const festd::span<const {{ column.typename }}> source)
        {
            FE_Assert(destination.m_count == source.size());

            const uint32_t pageIndex = destination.m_rowIndex / kRowsPerPage;
            const uint32_t localRowIndex = destination.m_rowIndex % kRowsPerPage;
            FE_Assert(pageIndex == (destination.m_rowIndex + destination.m_count - 1) / kRowsPerPage,
                      "The whole range must fit in a single page");

            DB::StoragePage* page = m_pages[pageIndex];
            m_database->MarkPageDirty(page);
            std::byte* storage = page->GetHostStorage();

            auto* destinationData = reinterpret_cast<{{ column.typename }}*>(storage + kOffset_{{ column.name }}) + localRowIndex;
            festd::copy(source, destinationData);
        }
        {% endfor -%}
    };
}
