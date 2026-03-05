#pragma once
#include <Shaders/Database/Base.hlsli>
#include <Shaders/Tables/Forwards.hlsli>
{% for include in table.includes['hlsl'] %}
#include {{ include }}
{% endfor %}
struct {{ table.name }}
{
    FE_CONSTEXPR uint32_t kRowsPerPage = {{ table.element_count_per_page }};
    {% for name, offset in table.offsets.items() %}
    FE_CONSTEXPR uint32_t kOffset_{{ name }} = {{ offset }};
    {%- endfor %}

    struct Row
    {
        {%- for column in table.columns %}
        DB::ElementHandle<{{ column.typename }}, kOffset_{{ column.name }}> {{ column.name }};
        {%- endfor %}
    };

    struct Instance
    {
        BufferPointer m_pageTableAddress;
    };

    static {{ table.name }} Create(Instance instance)
    {
        {{ table.name }} table;
        table.m_pageTableAddress = instance.m_pageTableAddress;
        return table;
    }

    Row ReadRow(const uint32_t rowIndex)
    {
        const uint32_t pageIndex = rowIndex / kRowsPerPage;
        const uint32_t localRowIndex = rowIndex % kRowsPerPage;

        const BufferPointer pageAddress = m_pageTableAddress.Read<BufferPointer>(pageIndex * sizeof(BufferPointer));

        Row row;
        {%- for column in table.columns %}
        row.{{ column.name }}.Setup(pageAddress, localRowIndex);
        {%- endfor %}
        return row;
    }

    Row ReadRow(const DB::Ref<{{ table.name }}> reference)
    {
        return ReadRow(reference.m_rowIndex);
    }

    BufferPointer m_pageTableAddress;
};
