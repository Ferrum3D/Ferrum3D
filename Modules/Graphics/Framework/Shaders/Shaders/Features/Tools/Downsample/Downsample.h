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
    };

FE_HOST_END_NAMESPACE
