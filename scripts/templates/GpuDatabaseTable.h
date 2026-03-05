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

        static constexpr uint32_t kElementCountPerPage = {{ table.element_count_per_page }};
        {% for name, offset in table.offsets.items() %}
        static constexpr uint32_t kOffset_{{ name }} = {{ offset }};
        {%- endfor %}

        static_assert(kElementCountPerPage > 0);

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
            : TableBase(database, kElementCountPerPage)
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

        [[nodiscard]] Row ReadRow(const uint32_t rowIndex)
        {
            const uint32_t pageIndex = rowIndex / kElementCountPerPage;
            const uint32_t localRowIndex = rowIndex % kElementCountPerPage;

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
            const uint32_t pageIndex = rowIndex / kElementCountPerPage;
            const uint32_t localRowIndex = rowIndex % kElementCountPerPage;

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
    };
}
