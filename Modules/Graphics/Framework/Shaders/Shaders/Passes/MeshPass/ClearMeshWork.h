#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::ClearMeshWork)
    FE_CONSTEXPR uint32_t kThreadCount = 256;

    struct Constants final
    {
        RWStructuredBufferDescriptor<uint32_t> m_counts;
        RWStructuredBufferDescriptor<uint32_t> m_cursors;
        uint32_t m_bucketCount;

        FE_RTTI_Reflect("7E4F82A9-69A7-4AFD-8AC5-3FA032DABBB3");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("388ED96C-7C0E-4D70-9233-30D3E126BE6C");
    };
FE_HOST_END_NAMESPACE
