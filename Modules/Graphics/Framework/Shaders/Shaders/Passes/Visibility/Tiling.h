#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>
#include <Shaders/Passes/Visibility/Contracts.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::Visibility)
#if FE_HOST
    using MeshPass::ViewData;
#endif
    FE_CONSTEXPR uint32_t kSortBlockSize = 256;
    FE_CONSTEXPR uint32_t kRadixBinCount = 16;

    struct SurfaceRun final
    {
        uint32_t m_group;
        uint32_t m_tile;
        uint32_t m_pixelOffset;
        uint32_t m_count;
        uint32_t m_evaluationSlot;
        uint32_t m_shadingRoute;
    };


    struct ShaderRange final
    {
        uint32_t m_firstWave;
        uint32_t m_waveCount;
    };


    struct TilingConstants final
    {
        StructuredBufferDescriptor<ViewData> m_view;
        Texture2DDescriptor<uint2> m_visibility;
        StructuredBufferDescriptor<uint2> m_input;
        RWStructuredBufferDescriptor<uint2> m_output;
        RWStructuredBufferDescriptor<uint32_t> m_histogram;
        RWStructuredBufferDescriptor<uint32_t> m_offsets;
        RWStructuredBufferDescriptor<uint32_t> m_pixels;
        RWStructuredBufferDescriptor<SurfaceRun> m_runs;
        RWStructuredBufferDescriptor<WaveCommand> m_waves;
        RWStructuredBufferDescriptor<ShaderRange> m_ranges;
        RWStructuredBufferDescriptor<DispatchArguments> m_arguments;
        RWStructuredBufferDescriptor<uint32_t> m_scan;
        RWStructuredBufferDescriptor<uint32_t> m_blockTotals;
        RWStructuredBufferDescriptor<uint32_t> m_blockOffsets;
        StructuredBufferDescriptor<uint32_t> m_histogramRead;
        StructuredBufferDescriptor<uint32_t> m_offsetsRead;
        StructuredBufferDescriptor<SurfaceRun> m_runsRead;
        StructuredBufferDescriptor<uint32_t> m_scanRead;
        StructuredBufferDescriptor<uint32_t> m_blockTotalsRead;
        StructuredBufferDescriptor<uint32_t> m_blockOffsetsRead;
        RWTexture2DDescriptor<uint32_t> m_g0;
        uint2 m_resolution;
        uint32_t m_firstTile;
        uint32_t m_tileCount;
        uint32_t m_pixelCount;
        uint32_t m_tilesPerRow;
        uint32_t m_shaderCount;
        uint32_t m_waveLaneCount;
        uint32_t m_shift;
        uint32_t m_segmented;
        FE_RTTI_Reflect("D2156239-F9FB-4508-A008-00EBB0F01084");
    };


    struct TilingPassDesc final
    {
        TilingConstants m_constants;
        Core::PassComputePipeline m_pipeline;
        FE_RTTI_Reflect("59B99AFD-33B4-41E1-A7D3-78D4E65FFB3E");
    };
FE_HOST_END_NAMESPACE
