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

        FE_RTTI_Reflect("FFB6B434-3BFA-4058-8192-259345DB489B");
    };

    struct PassDesc final
    {
        Constants m_constants;
        Core::PassColorTarget m_colorTarget;
        Core::PassGraphicsPipeline m_pipeline;

        FE_RTTI_Reflect("631D3D7C-40EF-4B50-A8F6-42EB2861045A");
    };

FE_HOST_END_NAMESPACE
