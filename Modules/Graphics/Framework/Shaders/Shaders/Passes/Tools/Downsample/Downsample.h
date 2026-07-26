#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Tools::Downsample)

    static const uint32_t kSpdMaxMipLevels = 12;

    struct SpdGlobalAtomicBuffer
    {
        uint32_t m_counter[6];
    };

    struct Constants
    {
        uint32_t m_mips;
        uint32_t m_numWorkGroups;
        uint2 m_workGroupOffset;
        float2 m_invInputSize;
        Texture2DArrayDescriptor<float4> m_input;
        GloballyCoherentRWStructuredBufferDescriptor<SpdGlobalAtomicBuffer> m_internalGlobalAtomic;
        GloballyCoherentRWTexture2DArrayDescriptor<float4> m_inputSrcMidMip;
        RWTexture2DArrayDescriptor<float4> m_inputSrcMips[kSpdMaxMipLevels + 1];
        SamplerDescriptor m_linearClamp;
        float m_padding;

        FE_RTTI_Reflect("1E74B92C-3D4D-4A75-AF29-F023496E19BF");
    };

    struct PassDesc
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("C9D2D096-0986-4DFB-B940-15C9AC6EA4AB");
    };

FE_HOST_END_NAMESPACE
