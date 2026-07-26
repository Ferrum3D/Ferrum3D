#pragma once
#include <Shaders/Base/Base.h>
#include <Shaders/Passes/MeshPass/MeshPass.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::OpaquePass)

    struct PassDesc final
    {
        MeshPass::Constants m_constants;
        Core::PassBufferAccess m_geometryBuffer;
        Core::PassColorTarget m_colorTarget;
        Core::PassDepthTarget m_depthTarget;
        Core::PassViewport m_viewport;
        Core::PassGraphicsPipeline m_pipeline;

        FE_RTTI_Reflect("36F8EE1D-C760-4977-A8CD-A8CC79F66803");
    };

FE_HOST_END_NAMESPACE
