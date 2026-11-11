#pragma once
#include <Shaders/Lighting/ViewLighting.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics)
    struct TransparencyLighting final
    {
        ConstantsPtr<ViewLighting> m_lighting;
        RWStructuredBufferDescriptor<uint32_t> m_shadowFeedback;
        FE_RTTI_Reflect("1B514CE8-CE70-4E01-B050-D411650650BE");
    };


    struct TransparencyResolveConstants final
    {
        Texture2DDescriptor<float4> m_accumulation;
        Texture2DDescriptor<float> m_revealage;
        RWTexture2DDescriptor<float3> m_color;
        uint2 m_resolution;
        FE_RTTI_Reflect("9B59A41A-C75B-452A-B41F-790705E9A653");
    };
FE_HOST_END_NAMESPACE
