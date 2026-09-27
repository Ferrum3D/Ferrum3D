#pragma once
#include <Shaders/Base/Base.h>
#include <Shaders/Passes/MeshPass/IndirectArguments.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::BuildIndirectArgs)

    struct Constants final
    {
        StructuredBufferDescriptor<MeshDispatchArguments> m_groupCounts;
        RWStructuredBufferDescriptor<MeshDispatchArguments> m_arguments;
        uint32_t m_groupCount;

        FE_RTTI_Reflect("3DF8562A-0782-4FE0-A359-781735EBD280");
    };

    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("83AA329F-9670-4F8C-A01A-B38102188E96");
    };

FE_HOST_END_NAMESPACE
