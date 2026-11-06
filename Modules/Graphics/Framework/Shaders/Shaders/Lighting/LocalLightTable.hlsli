#pragma once
#include <Shaders/Base/Packing.hlsli>
#include <Shaders/Lighting/LocalLighting.h>

struct DecodedLocalLight
{
    float4 m_positionRange;
    float4 m_colorCandela;
    float4 m_directionOuterCos;
    float4 m_cone;
};


DecodedLocalLight DecodeLocalLight(const float4 positionRange, const LocalLightShading shading)
{
    const bool isPoint = shading.m_coneAngles == kLocalPointLightCone;
    const float innerAngle = float(shading.m_coneAngles & 65535u) / kLocalLightConeAngleScale;
    const float outerAngle = float(shading.m_coneAngles >> 16) / kLocalLightConeAngleScale;
    DecodedLocalLight light;
    light.m_positionRange = positionRange;
    light.m_colorCandela = float4(Math::Pack::R11G11B10FloatToRGB32Float(shading.m_color), shading.m_colorMultiplier);
    light.m_directionOuterCos = float4(Math::Pack::Octahedral16SnormToNormal(shading.m_direction), isPoint ? -1 : cos(outerAngle));
    light.m_cone = float4(isPoint ? -1 : cos(innerAngle), 0, 0, 0);
    return light;
}


DecodedLocalLight LoadLocalLight(LocalLightingBinding binding, uint index)
{
    const LocalLightTable::Row row = binding.m_lights.ReadRow(index);
    return DecodeLocalLight(row.m_positionRange.Get(), row.m_shading.Get());
}
