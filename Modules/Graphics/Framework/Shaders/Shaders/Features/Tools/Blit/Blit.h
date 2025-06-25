#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Tools::Blit)

    struct Constants
    {
        float2 m_uvOffset;
        float2 m_uvScale;
        Texture2DDescriptor<float4> m_input;
        SamplerDescriptor m_sampler;
        float2 m_padding;
    };

FE_HOST_END_NAMESPACE
