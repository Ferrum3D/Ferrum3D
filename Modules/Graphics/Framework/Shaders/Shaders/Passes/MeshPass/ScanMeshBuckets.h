#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::ScanMeshBuckets)
    FE_CONSTEXPR uint32_t kThreadCount = kBucketsPerScanGroup;

    struct Constants final
    {
        StructuredBufferDescriptor<uint2> m_input;
        RWStructuredBufferDescriptor<uint2> m_output;
        RWStructuredBufferDescriptor<uint2> m_sums;
        uint32_t m_count;

        FE_RTTI_Reflect("B104CD64-92A0-4299-9AE4-0DF27B73CB75");
    };
FE_HOST_END_NAMESPACE
