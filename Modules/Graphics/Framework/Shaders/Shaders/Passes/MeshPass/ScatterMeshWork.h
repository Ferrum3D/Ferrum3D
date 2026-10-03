#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::ScatterMeshWork)
    struct Constants final
    {
        StructuredBufferDescriptor<InstanceClassification> m_classification;
        StructuredBufferDescriptor<uint32_t> m_instanceBuckets;
        StructuredBufferDescriptor<PipelineBucket> m_buckets;
        RWStructuredBufferDescriptor<uint32_t> m_cursors;
        RWStructuredBufferDescriptor<VisibleInstance> m_instances;
        RWStructuredBufferDescriptor<MeshletWorkChunk> m_work;

        FE_RTTI_Reflect("F43CDE27-BE2F-41A5-BA1E-64F6D377F746");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("171F7C24-A548-4D43-8CAC-DC79CD672599");
    };
FE_HOST_END_NAMESPACE
