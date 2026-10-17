#pragma once
#include <Shaders/Deferred/Contracts.h>
#include <Shaders/Scene/ViewData.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
    FE_CONSTEXPR uint32_t kSurfaceBlockSize = 256;

    struct ShaderRange final
    {
        uint32_t m_firstWave;
        uint32_t m_waveCount;
    };


    struct TilingConstants final
    {
        ByteAddressBufferDescriptor m_view;
        Texture2DDescriptor<uint2> m_visibility;
        StructuredBufferDescriptor<uint2> m_lookup;
        StructuredBufferDescriptor<uint32_t> m_bindings;
        StructuredBufferDescriptor<uint2> m_shaderBindings;
        RWStructuredBufferDescriptor<uint32_t> m_counts;
        RWStructuredBufferDescriptor<uint32_t> m_cursors;
        StructuredBufferDescriptor<uint32_t> m_countsRead;
        RWStructuredBufferDescriptor<uint32_t> m_pixels;
        RWStructuredBufferDescriptor<WaveCommand> m_waves;
        RWStructuredBufferDescriptor<ShaderRange> m_ranges;
        RWStructuredBufferDescriptor<DispatchArguments> m_arguments;
        StructuredBufferDescriptor<uint2> m_scanInput;
        RWStructuredBufferDescriptor<uint2> m_scanOutput;
        RWStructuredBufferDescriptor<uint2> m_scanSums;
        StructuredBufferDescriptor<uint2> m_parentOffsets;
        StructuredBufferDescriptor<uint2> m_offsets;
        StructuredBufferDescriptor<uint2> m_totals;
        RWTexture2DDescriptor<uint32_t> m_g0;
        Texture2DDescriptor<uint32_t> m_g0Read;
        uint2 m_resolution;
        uint32_t m_tilesPerRow;
        uint32_t m_tileCount;
        uint32_t m_bucketCount;
        uint32_t m_groupRowCount;
        uint32_t m_shaderCount;
        uint32_t m_waveLaneCount;
        uint32_t m_scanCount;
        uint32_t m_scanBuckets;
        uint32_t m_classifyShading;

        FE_RTTI_Reflect("D2156239-F9FB-4508-A008-00EBB0F01084");
    };


    struct TilingPassDesc final
    {
        TilingConstants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("59B99AFD-33B4-41E1-A7D3-78D4E65FFB3E");
    };
FE_HOST_END_NAMESPACE
