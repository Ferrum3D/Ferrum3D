#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>
#include <Shaders/Passes/MeshPass/IndirectArguments.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::FinalizeMeshletCommands)
    FE_CONSTEXPR uint32_t kThreadCount = 256;

    //! Splits the visible meshlets of each bucket into mesh shader dispatches of at most kMeshletsPerCommand groups.
    struct Constants final
    {
        StructuredBufferDescriptor<PipelineBucket> m_buckets;
        StructuredBufferDescriptor<uint32_t> m_meshletCounts;
        StructuredBufferDescriptor<BucketLayout> m_layouts;
        RWStructuredBufferDescriptor<MeshCommandRange> m_commands;
        RWStructuredBufferDescriptor<MeshDispatchArguments> m_arguments;
        uint32_t m_bucketCount;

        FE_RTTI_Reflect("9049885E-C514-40C6-9F2D-21AA94A9202C");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("37F91DFB-9BFD-489F-8E12-7DBFC3163B98");
    };
FE_HOST_END_NAMESPACE
