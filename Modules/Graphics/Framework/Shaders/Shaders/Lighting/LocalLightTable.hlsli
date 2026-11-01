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


BufferPointer GetLocalLightAddress(BufferPointer table, uint index)
{
    LocalLightTable::Instance instance;
    instance.m_pageTableAddress = table;
    return LocalLightTable::Create(instance).ReadRow(index).m_light.m_ptr;
}


//! The first 16 bytes of a packed light, enough for a sphere test.
float4 LoadLocalLightPositionRange(BufferPointer light)
{
    return light.Read<float4>(0);
}


//! Completes a packed light whose first 16 bytes were already loaded.
LocalLightData LoadLocalLightRemainder(BufferPointer light, const float4 positionRange)
{
    const uint4 remainder = light.Read<uint4>(16);
    LocalLightData packed;
    packed.m_positionRange = positionRange;
    packed.m_color = remainder.x;
    packed.m_colorMultiplier = asfloat(remainder.y);
    packed.m_direction = remainder.z;
    packed.m_coneAngles = remainder.w;
    return packed;
}


DecodedLocalLight DecodeLocalLight(const LocalLightData packed)
{
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


DecodedLocalLight LoadLocalLight(LocalLightingBinding binding, uint index)
{
    return DecodeLocalLight(GetLocalLightAddress(binding.m_lights, index).Read<LocalLightData>(0));
}
