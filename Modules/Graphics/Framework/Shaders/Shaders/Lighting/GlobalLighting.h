#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    struct GlobalLightingData final
    {
        float4 m_directionIlluminance;
        float4 m_colorSunRadius;
        float4 m_rayleighScaleHeight;
        float4 m_mieScatteringScaleHeight;
        float4 m_mieExtinctionAnisotropy;
        float4 m_ozoneAbsorptionCenter;
        float4 m_planetAtmosphereOzone;
        float4 m_groundAlbedo;
    };


    struct GlobalLightingBinding final
    {
        BufferPointer m_table FE_INIT({});
        uint32_t m_row FE_INIT(kInvalidIndex);
    };
FE_HOST_END_NAMESPACE
