#pragma once
#include <Shaders/Lighting/GlobalLighting.h>
#include <Shaders/Tables/GlobalLightingTable.hlsli>

GlobalLightingData LoadGlobalLighting(GlobalLightingBinding binding)
{
    if (binding.m_row == kInvalidIndex)
        return (GlobalLightingData)0;

    GlobalLightingTable::Instance instance;
    instance.m_pageTableAddress = binding.m_table;
    GlobalLightingTable table = GlobalLightingTable::Create(instance);
    return table.ReadRow(binding.m_row).m_lighting.Get();
}
