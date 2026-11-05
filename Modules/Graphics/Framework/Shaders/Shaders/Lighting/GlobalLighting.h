#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    struct GlobalLightingData final
    {
        float4 m_directionIlluminance;
        float4 m_colorSunRadius;
        float4 m_skyCoefficients[9];
        float4 m_skyRadiance;
        float4 m_sunIlluminance;
        float4 m_groundRadiance;
        FE_RTTI_Reflect("BCCE76B0-9747-4DE6-A642-3E6DD300E60B");
    };
FE_HOST_END_NAMESPACE
