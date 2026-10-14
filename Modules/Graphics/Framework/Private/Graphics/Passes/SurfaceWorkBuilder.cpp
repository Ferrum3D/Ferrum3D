#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>
#include <bit>

namespace FE::Graphics::Visibility
{
    static_assert(sizeof(TilingConstants) <= 128);
    static_assert(sizeof(SurfaceRun) == 24);

    namespace
    {
        FE_COMPUTE_PIPELINE_SET(EmitPixelsPipeline, "Shaders/Passes/Visibility/EmitSurfacePixels.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(HistogramPipeline, "Shaders/Passes/Visibility/RadixHistogram.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(OffsetsPipeline, "Shaders/Passes/Visibility/RadixOffsets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScatterPipeline, "Shaders/Passes/Visibility/RadixScatter.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(RunsPipeline, "Shaders/Passes/Visibility/BuildSurfaceRuns.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScanWavesPipeline, "Shaders/Passes/Visibility/ScanSurfaceWaves.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScanBlocksPipeline, "Shaders/Passes/Visibility/ScanSurfaceBlocks.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(EmitWavesPipeline, "Shaders/Passes/Visibility/EmitSurfaceWaves.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(RangesPipeline, "Shaders/Passes/Visibility/SurfaceRanges.cs.hlsl");


        TilingPassDesc* AllocateStep(Core::FrameGraph& graph, Core::ComputePipeline* pipeline, const TilingConstants& dimensions)
        {
            auto* pass = graph.AllocatePassData<TilingPassDesc>();
            pass->m_pipeline = pipeline;
            pass->m_constants = dimensions;
            return pass;
        }


        void RadixSort(Core::FrameGraph& graph, const SurfaceWork& work, const TilingConstants& dimensions, const bool segmented,
                       uint32_t& input)
        {
            for (uint32_t shift = 0; shift < 32; shift += 4)
            {
                auto* histogram = AllocateStep(graph, HistogramPipeline::GetPipeline(), dimensions);
                histogram->m_constants.m_input = graph.GetSRV(work.m_records[input].Get());
                histogram->m_constants.m_histogram = graph.GetUAV(work.m_histogram.Get());
                histogram->m_constants.m_shift = shift;

                graph.AddDispatchPass("SurfaceRadixHistogram",
                                      histogram,
                                      Core::ComputeWorkGroupCount(dimensions.m_pixelCount / kSortBlockSize));
                auto* offsets = AllocateStep(graph, OffsetsPipeline::GetPipeline(), dimensions);
                offsets->m_constants.m_histogramRead = graph.GetSRV(work.m_histogram.Get());
                offsets->m_constants.m_offsets = graph.GetUAV(work.m_offsets.Get());
                offsets->m_constants.m_segmented = segmented;

                graph.AddDispatchPass("SurfaceRadixOffsets",
                                      offsets,
                                      Core::ComputeWorkGroupCount(segmented ? dimensions.m_tileCount : 1));
                auto* scatter = AllocateStep(graph, ScatterPipeline::GetPipeline(), dimensions);
                scatter->m_constants.m_input = graph.GetSRV(work.m_records[input].Get());
                scatter->m_constants.m_output = graph.GetUAV(work.m_records[input ^ 1].Get());
                scatter->m_constants.m_offsetsRead = graph.GetSRV(work.m_offsets.Get());
                scatter->m_constants.m_shift = shift;
                graph.AddDispatchPass("SurfaceRadixScatter",
                                      scatter,
                                      Core::ComputeWorkGroupCount(dimensions.m_pixelCount / kSortBlockSize));
                input ^= 1;
            }
        }
    } // namespace


    uint32_t GetSurfaceTileCapacity(const uint32_t shaderCount, const uint64_t budget)
    {
        const uint64_t overhead = uint64_t(Math::Max(shaderCount, 1u)) * 40 + 4096;
        if (budget <= overhead)
            return 0;

        const uint64_t tiles = (budget - overhead) / kSurfaceScratchBytesPerTile;
        return std::bit_floor(static_cast<uint32_t>(Math::Min(tiles, uint64_t(64))));
    }


    SurfaceWork CreateSurfaceWork(Core::Device* device, const uint32_t shaderCount, const uint64_t budget)
    {
        SurfaceWork work;
        work.m_shaderCount = shaderCount;
        work.m_tileCapacity = GetSurfaceTileCapacity(shaderCount, budget);
        FE_Assert(work.m_tileCapacity > 0, "Surface scratch budget cannot hold one tile");
        const uint32_t pixels = work.m_tileCapacity * kTileSize * kTileSize;
        const uint32_t blocks = pixels / kSortBlockSize;
        const uint32_t slots = Math::Max(shaderCount, 1u);
        work.m_records[0] = Core::Buffer::CreateStructured<Vector2UInt>(device, "SurfaceRecords0", pixels);
        work.m_records[1] = Core::Buffer::CreateStructured<Vector2UInt>(device, "SurfaceRecords1", pixels);
        work.m_histogram = Core::Buffer::CreateStructured<uint32_t>(device, "SurfaceHistogram", blocks * kRadixBinCount);
        work.m_offsets = Core::Buffer::CreateStructured<uint32_t>(device, "SurfaceRadixOffsets", blocks * kRadixBinCount);
        work.m_pixels = Core::Buffer::CreateStructured<uint32_t>(device, "SurfacePixels", pixels);
        work.m_runs = Core::Buffer::CreateStructured<SurfaceRun>(device, "SurfaceRuns", pixels);
        work.m_waves = Core::Buffer::CreateStructured<WaveCommand>(device, "SurfaceWaves", pixels);
        work.m_ranges = Core::Buffer::CreateStructured<ShaderRange>(device, "SurfaceRanges", slots);
        work.m_arguments = Core::Buffer::CreateStructured<DispatchArguments>(device, "SurfaceArguments", slots);
        work.m_scan = Core::Buffer::CreateStructured<uint32_t>(device, "SurfaceScan", pixels);
        work.m_blockTotals = Core::Buffer::CreateStructured<uint32_t>(device, "SurfaceBlockTotals", blocks);
        work.m_blockOffsets = Core::Buffer::CreateStructured<uint32_t>(device, "SurfaceBlockOffsets", blocks);
        work.m_scratchBytes = work.m_tileCapacity * kSurfaceScratchBytesPerTile + uint64_t(slots) * 40 + 4096;
        FE_Assert(work.m_scratchBytes <= budget);
        return work;
    }


    void BuildSurfaceWork(Core::FrameGraph& graph, Core::Buffer* view, Core::Texture* visibility, Core::Texture* g0,
                          const SurfaceWork& work, const uint32_t firstTile, const uint32_t tileCount,
                          const uint32_t waveLaneCount)
    {
        FE_Assert(tileCount > 0 && tileCount <= work.m_tileCapacity);
        FE_Assert(waveLaneCount == 32 || waveLaneCount == 64);
        TilingConstants dimensions{};
        dimensions.m_resolution = { visibility->GetDesc().m_width, visibility->GetDesc().m_height };
        dimensions.m_firstTile = firstTile;
        dimensions.m_tileCount = tileCount;
        dimensions.m_pixelCount = tileCount * kTileSize * kTileSize;
        dimensions.m_tilesPerRow = Math::CeilDivide(dimensions.m_resolution.x, kTileSize);
        dimensions.m_shaderCount = work.m_shaderCount;
        dimensions.m_waveLaneCount = waveLaneCount;
        const uint32_t groups = dimensions.m_pixelCount / kSortBlockSize;

        auto* emit = AllocateStep(graph, EmitPixelsPipeline::GetPipeline(), dimensions);
        emit->m_constants.m_view = graph.GetSRV(view);
        emit->m_constants.m_visibility = graph.GetSRV(Core::TextureView::Create(visibility));
        emit->m_constants.m_output = graph.GetUAV(work.m_records[0].Get());
        emit->m_constants.m_g0 = graph.GetUAV(Core::TextureView::Create(g0));
        graph.AddDispatchPass("EmitSurfacePixels", emit, Core::ComputeWorkGroupCount(groups));
        uint32_t input = 0;
        RadixSort(graph, work, dimensions, true, input);

        auto* runs = AllocateStep(graph, RunsPipeline::GetPipeline(), dimensions);
        runs->m_constants.m_view = graph.GetSRV(view);
        runs->m_constants.m_input = graph.GetSRV(work.m_records[input].Get());
        runs->m_constants.m_output = graph.GetUAV(work.m_records[input ^ 1].Get());
        runs->m_constants.m_runs = graph.GetUAV(work.m_runs.Get());
        runs->m_constants.m_pixels = graph.GetUAV(work.m_pixels.Get());
        graph.AddDispatchPass("BuildSurfaceRuns", runs, Core::ComputeWorkGroupCount(groups));
        input ^= 1;
        RadixSort(graph, work, dimensions, false, input);

        auto* scan = AllocateStep(graph, ScanWavesPipeline::GetPipeline(), dimensions);
        scan->m_constants.m_input = graph.GetSRV(work.m_records[input].Get());
        scan->m_constants.m_runsRead = graph.GetSRV(work.m_runs.Get());
        scan->m_constants.m_scan = graph.GetUAV(work.m_scan.Get());
        scan->m_constants.m_blockTotals = graph.GetUAV(work.m_blockTotals.Get());
        graph.AddDispatchPass("ScanSurfaceWaves", scan, Core::ComputeWorkGroupCount(groups));
        auto* blocks = AllocateStep(graph, ScanBlocksPipeline::GetPipeline(), dimensions);
        blocks->m_constants.m_blockTotalsRead = graph.GetSRV(work.m_blockTotals.Get());
        blocks->m_constants.m_blockOffsets = graph.GetUAV(work.m_blockOffsets.Get());

        graph.AddDispatchPass("ScanSurfaceBlocks", blocks, Core::ComputeWorkGroupCount(1));
        auto* waves = AllocateStep(graph, EmitWavesPipeline::GetPipeline(), dimensions);
        waves->m_constants.m_input = graph.GetSRV(work.m_records[input].Get());
        waves->m_constants.m_runsRead = graph.GetSRV(work.m_runs.Get());
        waves->m_constants.m_scanRead = graph.GetSRV(work.m_scan.Get());
        waves->m_constants.m_blockOffsetsRead = graph.GetSRV(work.m_blockOffsets.Get());
        waves->m_constants.m_waves = graph.GetUAV(work.m_waves.Get());
        graph.AddDispatchPass("EmitSurfaceWaves", waves, Core::ComputeWorkGroupCount(groups));
        auto* ranges = AllocateStep(graph, RangesPipeline::GetPipeline(), dimensions);
        ranges->m_constants.m_input = graph.GetSRV(work.m_records[input].Get());
        ranges->m_constants.m_scanRead = graph.GetSRV(work.m_scan.Get());
        ranges->m_constants.m_blockTotalsRead = graph.GetSRV(work.m_blockTotals.Get());
        ranges->m_constants.m_blockOffsetsRead = graph.GetSRV(work.m_blockOffsets.Get());
        ranges->m_constants.m_ranges = graph.GetUAV(work.m_ranges.Get());
        ranges->m_constants.m_arguments = graph.GetUAV(work.m_arguments.Get());
        graph.AddDispatchPass("SurfaceShaderRanges",
                              ranges,
                              Core::ComputeWorkGroupCount(Math::CeilDivide(work.m_shaderCount, kSortBlockSize)));
    }
} // namespace FE::Graphics::Visibility
