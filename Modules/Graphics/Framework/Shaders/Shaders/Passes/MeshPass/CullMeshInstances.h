#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::CullMeshInstances)
    struct Constants final
    {
        StructuredBufferDescriptor<ViewData> m_view;
        StructuredBufferDescriptor<CullDispatch> m_dispatches;
        RWStructuredBufferDescriptor<InstanceClassification> m_classification;
        RWStructuredBufferDescriptor<uint32_t> m_counts;

        FE_RTTI_Reflect("818EF35C-7444-45F2-B6A5-8C97A016F41C");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassBufferAccess m_routing;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("C4F922E3-773F-41CE-867A-C4B0B34DC424");
    };
FE_HOST_END_NAMESPACE
