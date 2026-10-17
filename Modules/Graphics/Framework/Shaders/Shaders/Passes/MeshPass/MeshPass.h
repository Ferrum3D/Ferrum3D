#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass)
    struct Constants final
    {
        StructuredBufferDescriptor<MeshViewData> m_view;
        uint32_t m_commandIndex;
        FE_RTTI_Reflect("9565427F-FE72-49CD-A99E-2A4082D638F7");
    };

    struct PassDesc final
    {
        Constants m_constants;
        Core::PassGraphicsPipeline m_pipeline;
        Core::PassIndirectArgs m_arguments;
        FE_RTTI_Reflect("C3621D29-94E8-42DA-A4D8-BA3FD6EBEC1E");
    };
FE_HOST_END_NAMESPACE
