#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::CountMeshInstances)
    struct Constants final
    {
        StructuredBufferDescriptor<InstanceClassification> m_classification;
        StructuredBufferDescriptor<PipelineRouting> m_routing;
        RWStructuredBufferDescriptor<uint32_t> m_instanceBuckets;
        RWStructuredBufferDescriptor<uint32_t> m_counts;
        uint2 m_drawTagMask;

        FE_RTTI_Reflect("0F0748C2-1D73-4B58-A9E2-48E75C6C51F0");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("7F32193B-CC9C-46EA-A206-B5DD9DC5D4FD");
    };
FE_HOST_END_NAMESPACE
