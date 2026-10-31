#pragma once
#include <Shaders/Lighting/LocalLighting.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    struct ForwardLightingData final
    {
        GlobalLightingBinding m_global;
        LocalLightingBinding m_local;
    };


    struct TransparencyResolveConstants final
    {
        Texture2DDescriptor<float4> m_accumulation;
        Texture2DDescriptor<float> m_revealage;
        RWTexture2DDescriptor<float3> m_color;
        uint2 m_resolution;
        FE_RTTI_Reflect("9B59A41A-C75B-452A-B41F-790705E9A653");
    };


    struct TransparencyResolvePassDesc final
    {
        TransparencyResolveConstants m_constants;
        Core::PassComputePipeline m_pipeline;
        FE_RTTI_Reflect("48292333-CB86-41B8-926E-238603DDD8A8");
    };
FE_HOST_END_NAMESPACE
