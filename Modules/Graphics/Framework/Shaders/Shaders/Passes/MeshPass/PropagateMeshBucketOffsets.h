#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::PropagateMeshBucketOffsets)
    FE_CONSTEXPR uint32_t kThreadCount = 256;

    struct Constants final
    {
        StructuredBufferDescriptor<uint2> m_input;
        RWStructuredBufferDescriptor<uint2> m_output;
        uint32_t m_count;

        FE_RTTI_Reflect("98F0A8E2-053C-4F16-AC99-ABF61FDB4E67");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("33EFA1B1-5190-466F-A50C-23794A74045A");
    };
FE_HOST_END_NAMESPACE
