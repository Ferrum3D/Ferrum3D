#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    struct ViewData final
    {
        float4x4 m_viewProjection;
        float4x4 m_inverseViewProjection;
        float4 m_cameraPosition;
        float4 m_viewport;
        float4 m_backgroundColor;
    };
FE_HOST_END_NAMESPACE
