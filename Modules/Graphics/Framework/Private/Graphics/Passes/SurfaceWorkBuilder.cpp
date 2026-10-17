#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>

namespace FE::Graphics::Visibility
{
    static_assert(sizeof(TilingConstants) <= 128);

    namespace
    {
        FE_COMPUTE_PIPELINE_SET(ClearPipeline, "Shaders/Deferred/ClearSurfaceWork.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(CountPipeline, "Shaders/Deferred/CountSurfacePixels.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScanPipeline, "Shaders/Deferred/ScanSurfaceBuckets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(PropagatePipeline, "Shaders/Deferred/PropagateSurfaceOffsets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScatterPipeline, "Shaders/Deferred/ScatterSurfacePixels.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(CommandsPipeline, "Shaders/Deferred/EmitSurfaceCommands.cs.hlsl");


        Core::ComputeWorkGroupCount DispatchSize(const uint32_t threads)
        {
            const uint32_t groups = Math::Max(Math::CeilDivide(threads, kSurfaceBlockSize), 1u);
            return Vector2UInt(Math::Min(groups, 65535u), Math::CeilDivide(groups, 65535u));
        }


        TilingPassDesc* AllocateStep(Core::FrameGraph& graph, Core::ComputePipeline* pipeline, const TilingConstants& dimensions)
        {
            auto* pass = graph.AllocatePassData<TilingPassDesc>();
            pass->m_pipeline = pipeline;
            pass->m_constants = dimensions;
            return pass;
        }
    } // namespace


    SurfaceWork CreateSurfaceWork(Core::Device* device, const Vector2UInt resolution,
                                  const festd::span<const SurfaceBinding> bindings, const uint32_t groupRowCount,
                                  const uint32_t shaderCount, const uint32_t waveLaneCount)
    {
        FE_Assert(waveLaneCount == 32 || waveLaneCount == 64);
        FE_Assert(resolution.x > 0 && resolution.y > 0);

        SurfaceWork work;
        work.m_resolution = resolution;
        work.m_shaderCount = shaderCount;
        work.m_waveLaneCount = waveLaneCount;

        const Vector2UInt tileCount = Math::CeilDivide(resolution, kTileSize);
        const uint32_t tiles = tileCount.x * tileCount.y;
        const uint32_t pixels = tiles * kTileSize * kTileSize;
        const uint32_t buckets = tiles * bindings.size();

        work.m_tileCount = tiles;
        work.m_bucketCount = buckets;
        // Each nonempty bucket may add one partial wave beyond the full-wave pixel bound.
        work.m_waveCapacity = Math::Min(pixels, Math::CeilDivide(pixels, waveLaneCount) + Math::Min(pixels, buckets));
        if (bindings.empty())
            work.m_waveCapacity = 0;

        work.m_lookupData.resize(Math::Max(groupRowCount, 1u), Vector2UInt(kInvalidIndex, 0));
        work.m_shaderBindingData.resize(Math::Max(shaderCount, 1u), Vector2UInt(0, 0));

        festd::vector<SurfaceBinding> sorted;
        sorted.assign(bindings.begin(), bindings.end());
        festd::sort(sorted, [](const SurfaceBinding& lhs, const SurfaceBinding& rhs) {
            return lhs.m_shaderSlot != rhs.m_shaderSlot ? lhs.m_shaderSlot < rhs.m_shaderSlot : lhs.m_group < rhs.m_group;
        });

        work.m_bindingData.resize(Math::Max(sorted.size(), 1u), kInvalidIndex);

        // Bucket order is shader, tile, then binding, keeping each shader's commands contiguous.
        uint32_t firstBinding = 0;
        for (uint32_t slot = 0; slot < shaderCount; ++slot)
        {
            uint32_t end = firstBinding;
            while (end < sorted.size() && sorted[end].m_shaderSlot == slot)
                ++end;

            const uint32_t count = end - firstBinding;
            work.m_shaderBindingData[slot] = Vector2UInt(firstBinding, count);
            for (uint32_t index = firstBinding; index < end; ++index)
            {
                const uint32_t group = sorted[index].m_group;
                FE_Assert(group < groupRowCount && work.m_lookupData[group].x == kInvalidIndex);
                work.m_lookupData[group] = Vector2UInt(firstBinding * work.m_tileCount + index - firstBinding, count);
                work.m_bindingData[index] = group;
            }

            firstBinding = end;
        }

        FE_Assert(firstBinding == sorted.size(), "Surface binding has an invalid shader slot");
        work.m_lookup = Core::Buffer::CreateByteAddress(device, "SurfaceBindingLookup", 8 * work.m_lookupData.size());
        work.m_bindings = Core::Buffer::CreateByteAddress(device, "SurfaceBindings", 4 * work.m_bindingData.size());
        work.m_shaderBindings =
            Core::Buffer::CreateByteAddress(device, "SurfaceShaderBindings", 8 * work.m_shaderBindingData.size());
        work.m_counts = Core::Buffer::CreateByteAddress(device, "SurfaceCounts", 4 * Math::Max(work.m_bucketCount, 1u));
        work.m_cursors = Core::Buffer::CreateByteAddress(device, "SurfaceCursors", 4 * Math::Max(work.m_bucketCount, 1u));
        work.m_pixels =
            Core::Buffer::CreateByteAddress(device, "SurfacePixels", 4 * Math::Max(bindings.empty() ? 0u : pixels, 1u));
        work.m_waves = Core::Buffer::CreateByteAddress(device, "SurfaceWaves", 16 * Math::Max(work.m_waveCapacity, 1u));
        work.m_ranges = Core::Buffer::CreateByteAddress(device, "SurfaceRanges", 8 * Math::Max(shaderCount, 1u));
        work.m_arguments = Core::Buffer::CreateByteAddress(device, "SurfaceArguments", 12 * Math::Max(shaderCount, 1u));

        uint32_t count = Math::Max(work.m_bucketCount, 1u);

        for (;;)
        {
            const uint32_t blocks = Math::CeilDivide(count, kSurfaceBlockSize);
            SurfaceScanLevel level;
            level.m_count = count;
            level.m_offsets = Core::Buffer::CreateByteAddress(device, "SurfaceOffsets", 8 * count);
            level.m_sums = Core::Buffer::CreateByteAddress(device, "SurfaceBlockSums", 8 * blocks);
            work.m_scanLevels.push_back(level);
            if (blocks == 1)
                break;

            count = blocks;
        }

        return work;
    }


    void BuildSurfaceWork(Core::FrameGraph& graph, Core::RingUploader& uploader, Core::Buffer* view, Core::Texture* visibility,
                          Core::Texture* g0, SurfaceWork& work, const bool classifyShading, Core::ComputePipeline* countPipeline,
                          Core::ComputePipeline* scatterPipeline)
    {
        FE_Assert(g0->GetDesc().GetSize2D() == work.m_resolution);

        FE_Verify(uploader.UploadArray(graph, work.m_lookup.Get(), festd::span(work.m_lookupData)));
        FE_Verify(uploader.UploadArray(graph, work.m_bindings.Get(), festd::span(work.m_bindingData)));
        FE_Verify(uploader.UploadArray(graph, work.m_shaderBindings.Get(), festd::span(work.m_shaderBindingData)));

        TilingConstants dimensions{};
        dimensions.m_resolution = work.m_resolution;
        dimensions.m_tilesPerRow = Math::CeilDivide(work.m_resolution.x, kTileSize);
        dimensions.m_tileCount = work.m_tileCount;
        dimensions.m_bucketCount = work.m_bucketCount;
        dimensions.m_groupRowCount = work.m_lookupData.size();
        dimensions.m_shaderCount = work.m_shaderCount;
        dimensions.m_waveLaneCount = work.m_waveLaneCount;
        dimensions.m_classifyShading = classifyShading;

        // Reset bucket counters before counting visible pixels.
        auto* clear = AllocateStep(graph, ClearPipeline::GetPipeline(), dimensions);
        clear->m_constants.m_counts = graph.GetUAV(work.m_counts.Get());
        clear->m_constants.m_cursors = graph.GetUAV(work.m_cursors.Get());

        graph.AddDispatchPass("ClearSurfaceWork", clear, DispatchSize(work.m_bucketCount));

        auto* count = AllocateStep(graph, countPipeline ? countPipeline : CountPipeline::GetPipeline(), dimensions);
        if (classifyShading)
        {
            count->m_constants.m_g0Read = graph.GetSRV(g0);
        }
        else
        {
            count->m_constants.m_view = graph.GetSRV(view);
            count->m_constants.m_visibility = graph.GetSRV(visibility);
        }

        count->m_constants.m_lookup = graph.GetSRV(work.m_lookup.Get());
        count->m_constants.m_counts = graph.GetUAV(work.m_counts.Get());

        const Core::ComputeWorkGroupCount screenGroups = Math::CeilDivide(work.m_resolution, 16u);
        graph.AddDispatchPass("CountSurfacePixels", count, screenGroups);

        // Scan bucket counts hierarchically, then propagate parent prefixes back to each tile.
        for (uint32_t index = 0; index < work.m_scanLevels.size(); ++index)
        {
            const SurfaceScanLevel& level = work.m_scanLevels[index];
            auto* scan = AllocateStep(graph, ScanPipeline::GetPipeline(), dimensions);
            scan->m_constants.m_scanCount = level.m_count;
            scan->m_constants.m_scanBuckets = index == 0;
            if (index == 0)
                scan->m_constants.m_countsRead = graph.GetSRV(work.m_counts.Get());
            else
                scan->m_constants.m_scanInput = graph.GetSRV(work.m_scanLevels[index - 1].m_sums.Get());

            scan->m_constants.m_scanOutput = graph.GetUAV(level.m_offsets.Get());
            scan->m_constants.m_scanSums = graph.GetUAV(level.m_sums.Get());
            graph.AddDispatchPass("ScanSurfaceBuckets", scan, DispatchSize(level.m_count));
        }

        for (uint32_t index = work.m_scanLevels.size() - 1; index > 0; --index)
        {
            const SurfaceScanLevel& child = work.m_scanLevels[index - 1];
            auto* propagate = AllocateStep(graph, PropagatePipeline::GetPipeline(), dimensions);
            propagate->m_constants.m_scanCount = child.m_count;
            propagate->m_constants.m_scanOutput = graph.GetUAV(child.m_offsets.Get());
            propagate->m_constants.m_parentOffsets = graph.GetSRV(work.m_scanLevels[index].m_offsets.Get());
            graph.AddDispatchPass("PropagateSurfaceOffsets", propagate, DispatchSize(child.m_count));
        }

        // Scatter tile-local pixel addresses into contiguous material runs.
        auto* scatter = AllocateStep(graph, scatterPipeline ? scatterPipeline : ScatterPipeline::GetPipeline(), dimensions);
        if (classifyShading)
        {
            scatter->m_constants.m_g0Read = graph.GetSRV(g0);
        }
        else
        {
            scatter->m_constants.m_view = graph.GetSRV(view);
            scatter->m_constants.m_visibility = graph.GetSRV(visibility);
        }

        scatter->m_constants.m_lookup = graph.GetSRV(work.m_lookup.Get());
        scatter->m_constants.m_offsets = graph.GetSRV(work.m_scanLevels.front().m_offsets.Get());
        scatter->m_constants.m_cursors = graph.GetUAV(work.m_cursors.Get());
        scatter->m_constants.m_pixels = graph.GetUAV(work.m_pixels.Get());
        graph.AddDispatchPass("ScatterSurfacePixels", scatter, screenGroups);

        // Divide each run into waves and emit one indirect dispatch per registered shader.
        auto* commands = AllocateStep(graph, CommandsPipeline::GetPipeline(), dimensions);
        commands->m_constants.m_offsets = graph.GetSRV(work.m_scanLevels.front().m_offsets.Get());
        commands->m_constants.m_totals = graph.GetSRV(work.m_scanLevels.back().m_sums.Get());
        commands->m_constants.m_countsRead = graph.GetSRV(work.m_counts.Get());
        commands->m_constants.m_bindings = graph.GetSRV(work.m_bindings.Get());
        commands->m_constants.m_shaderBindings = graph.GetSRV(work.m_shaderBindings.Get());
        commands->m_constants.m_waves = graph.GetUAV(work.m_waves.Get());
        commands->m_constants.m_ranges = graph.GetUAV(work.m_ranges.Get());
        commands->m_constants.m_arguments = graph.GetUAV(work.m_arguments.Get());
        graph.AddDispatchPass("EmitSurfaceCommands", commands, DispatchSize(Math::Max(work.m_waveCapacity, work.m_shaderCount)));
    }
} // namespace FE::Graphics::Visibility
