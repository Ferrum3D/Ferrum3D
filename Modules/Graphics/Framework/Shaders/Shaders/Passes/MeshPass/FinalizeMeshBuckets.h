#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>
#include <Shaders/Passes/MeshPass/IndirectArguments.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::FinalizeMeshBuckets)
    FE_CONSTEXPR uint32_t kThreadCount = 256;

    struct Constants final
    {
        StructuredBufferDescriptor<uint2> m_offsets;
        StructuredBufferDescriptor<uint32_t> m_counts;
        StructuredBufferDescriptor<BucketLayout> m_layouts;
        RWStructuredBufferDescriptor<PipelineBucket> m_buckets;
        RWStructuredBufferDescriptor<MeshCommandRange> m_commands;
        RWStructuredBufferDescriptor<MeshDispatchArguments> m_arguments;
        uint32_t m_bucketCount;

        FE_RTTI_Reflect("2B82E380-8B59-44EB-BBAC-DA2970D24BD4");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("9FB458CC-A1DE-4758-B0AC-D0A818BB01DF");
    };
FE_HOST_END_NAMESPACE
