#pragma once
#include <Shaders/Base/Base.h>
#include <Shaders/Passes/MeshPass/MeshPass.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::DepthPrepass)

    struct PassDesc final
    {
        MeshPass::Constants m_constants;
        Core::PassBufferAccess m_geometryBuffer;
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;
        Core::PassGraphicsPipeline m_pipeline;

        FE_RTTI_Reflect("A86B4026-4C36-45CE-B6E7-0431BF656B84");
    };

FE_HOST_END_NAMESPACE
