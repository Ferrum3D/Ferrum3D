#pragma once
#include <Shaders/Base/Base.h>
#include <Shaders/Passes/MeshPass/MeshPass.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::DepthPrepass)

    struct PassDesc final
    {
        MeshPass::Constants m_constants;
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;
        Core::PassGraphicsPipeline m_pipeline;
    };

FE_HOST_END_NAMESPACE

FE_DECLARE_PASS_DATA(FE::Graphics::DepthPrepass::PassDesc);
