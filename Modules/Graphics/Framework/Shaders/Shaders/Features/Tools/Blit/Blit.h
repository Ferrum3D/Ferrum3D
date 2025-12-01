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

    struct PassDesc final
    {
        Constants m_constants;
        Core::PassColorTarget m_colorTarget;
    };

FE_HOST_END_NAMESPACE

FE_DECLARE_PASS_DATA(FE::Graphics::Tools::Blit::Constants);
FE_DECLARE_PASS_DATA(FE::Graphics::Tools::Blit::PassDesc);
