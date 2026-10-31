#pragma once
#include <Shaders/Base/Packing.hlsli>
#include <Shaders/Tables/LocalLightTable.hlsli>

struct DecodedLocalLight
{
    float4 m_positionRange;
    float4 m_colorCandela;
    float4 m_directionOuterCos;
    float4 m_cone;
};


DecodedLocalLight LoadLocalLight(LocalLightingBinding binding, uint index)
{
    LocalLightTable::Instance instance;
    instance.m_pageTableAddress = binding.m_lights;
    const LocalLightData packed = LocalLightTable::Create(instance).ReadRow(index).m_light.Get();
    const bool isPoint = packed.m_coneAngles == kLocalPointLightCone;
    const float innerAngle = float(packed.m_coneAngles & 65535u) / kLocalLightConeAngleScale;
    const float outerAngle = float(packed.m_coneAngles >> 16) / kLocalLightConeAngleScale;
    DecodedLocalLight light;
    light.m_positionRange = packed.m_positionRange;
    light.m_colorCandela = float4(Math::Pack::R11G11B10FloatToRGB32Float(packed.m_color), packed.m_colorMultiplier);
    light.m_directionOuterCos = float4(Math::Pack::Octahedral16SnormToNormal(packed.m_direction), isPoint ? -1 : cos(outerAngle));
    light.m_cone = float4(isPoint ? -1 : cos(innerAngle), 0, 0, 0);
    return light;
}
