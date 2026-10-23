#pragma once
#include <Shaders/Base/Base.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Tools::DepthPyramid)

    //! The depth target is treated as padded to powers of two per axis. Mip zero has half the padded size, so every pyramid
    //! texel reduces exactly 2x2 texels of the level above. Padding replicates the edge texels of the depth target.
    FE_CONSTEXPR uint32_t kMaxMipCount = 15;

    //! The first pass reduces 32x32 depth texels per group in LDS and writes this many mips of both pyramids. Pyramids are at
    //! least one group wide on both axes, so the tile pass always writes all of them.
    FE_CONSTEXPR uint32_t kTileGroupSize = 16;
    FE_CONSTEXPR uint32_t kTileMipCount = 5;

    //! The single-pass downsampler writes the remaining mips from the last tile mip.
    FE_CONSTEXPR uint32_t kSpdMaxMipCount = 12;
    FE_CONSTEXPR uint32_t kSpdMidMip = 5;
    FE_CONSTEXPR uint32_t kSpdSourceTileSize = 64;

    //! The tile pass clears one completion counter per pyramid for the single-pass downsampler.
    FE_CONSTEXPR uint32_t kSpdCounterCount = 2;


    struct TileConstants final
    {
        Texture2DDescriptor<float> m_depth;
        SamplerDescriptor m_sampler;
        float2 m_invDepthSize;
        RWStructuredBufferDescriptor<uint32_t> m_spdCounters;
        RWTexture2DDescriptor<float> m_farthest[kTileMipCount];
        RWTexture2DDescriptor<float> m_closest[kTileMipCount];

        FE_RTTI_Reflect("5E568E19-7A5C-4EA9-B42D-F4E39D109B93");
    };


    struct TilePassDesc final
    {
        TileConstants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("DE08D889-A309-4F22-B99D-E94C8BF17EEF");
    };


    //! Reduces one pyramid from mip kTileMipCount - 1 down to 1x1. Specialization constants select the reduction and the
    //! number of mips.
    struct SpdConstants final
    {
        Texture2DDescriptor<float> m_source;
        SamplerDescriptor m_sampler;
        float2 m_invSourceSize;
        uint2 m_sourceSize;
        uint32_t m_workGroupCount;
        uint32_t m_counterIndex;
        GloballyCoherentRWStructuredBufferDescriptor<uint32_t> m_atomicCounter;
        GloballyCoherentRWTexture2DDescriptor<float> m_midMip;
        RWTexture2DDescriptor<float> m_mips[kSpdMaxMipCount];

        FE_RTTI_Reflect("C7F6FC3C-9BB0-413E-84F3-7155B6EE13C8");
    };


    struct SpdPassDesc final
    {
        SpdConstants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("EF57256D-AE4E-49B2-BA5E-3BD75C72B666");
    };

FE_HOST_END_NAMESPACE
