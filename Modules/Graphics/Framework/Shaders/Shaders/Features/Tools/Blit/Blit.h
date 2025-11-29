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

FE_RTTI_Reflect(FE::Graphics::Tools::Blit::Constants, "E713C84A-EF63-43BC-8BFD-E2AF0FE55171");
