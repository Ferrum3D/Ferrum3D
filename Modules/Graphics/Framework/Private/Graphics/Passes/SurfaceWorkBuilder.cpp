#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/ResourcePool.h>
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
            return Core::ComputeWorkGroupCount(Vector2UInt(Math::Min(groups, 65535u), Math::CeilDivide(groups, 65535u)));
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
        const uint64_t tiles = uint64_t(Math::CeilDivide(resolution.x, kTileSize)) * Math::CeilDivide(resolution.y, kTileSize);
        const uint64_t pixels = tiles * kTileSize * kTileSize;
        const uint64_t buckets = tiles * bindings.size();
        FE_Assert(pixels <= Constants::kMaxU32 / 4 && buckets <= Constants::kMaxU32 / 8);
        FE_Assert(groupRowCount <= Constants::kMaxU32 / 8 && shaderCount <= Constants::kMaxU32 / 12);
        work.m_tileCount = static_cast<uint32_t>(tiles);
        work.m_bucketCount = static_cast<uint32_t>(buckets);
        work.m_waveCapacity = static_cast<uint32_t>(
            Math::Min(pixels, Math::CeilDivide(pixels, uint64_t(waveLaneCount)) + Math::Min(pixels, buckets)));
        if (bindings.empty())
            work.m_waveCapacity = 0;
        work.m_sourceBindings.assign(bindings.begin(), bindings.end());
        work.m_lookupData.resize(Math::Max(groupRowCount, 1u), Vector2UInt(kInvalidIndex, 0));
        work.m_shaderBindingData.resize(Math::Max(shaderCount, 1u), Vector2UInt(0, 0));
        festd::vector<SurfaceBinding> sorted;
        sorted.assign(bindings.begin(), bindings.end());
        eastl::sort(sorted.begin(), sorted.end(), [](const SurfaceBinding& lhs, const SurfaceBinding& rhs) {
            return lhs.m_shaderSlot != rhs.m_shaderSlot ? lhs.m_shaderSlot < rhs.m_shaderSlot : lhs.m_group < rhs.m_group;
        });
        work.m_bindingData.resize(Math::Max(sorted.size(), 1u), kInvalidIndex);
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
        const auto create = [&](const char* name, const uint32_t stride, const uint32_t count) {
            const uint64_t bytes = uint64_t(stride) * Math::Max(count, 1u);
            FE_Assert(bytes <= Constants::kMaxU32, "Surface buffer exceeds addressable size");
            work.m_scratchBytes += bytes;
            return Core::Buffer::CreateByteAddress(device, Env::Name(name), static_cast<uint32_t>(bytes));
        };
        work.m_lookup = create("SurfaceBindingLookup", 8, work.m_lookupData.size());
        work.m_bindings = create("SurfaceBindings", 4, work.m_bindingData.size());
        work.m_shaderBindings = create("SurfaceShaderBindings", 8, work.m_shaderBindingData.size());
        work.m_counts = create("SurfaceCounts", 4, work.m_bucketCount);
        work.m_cursors = create("SurfaceCursors", 4, work.m_bucketCount);
        work.m_pixels = create("SurfacePixels", 4, bindings.empty() ? 0 : static_cast<uint32_t>(pixels));
        work.m_waves = create("SurfaceWaves", 16, work.m_waveCapacity);
        work.m_ranges = create("SurfaceRanges", 8, shaderCount);
        work.m_arguments = create("SurfaceArguments", 12, shaderCount);
        uint32_t count = Math::Max(work.m_bucketCount, 1u);
        for (;;)
        {
            const uint32_t blocks = Math::CeilDivide(count, kSurfaceBlockSize);
            SurfaceScanLevel level;
            level.m_count = count;
            level.m_offsets = create("SurfaceOffsets", 8, count);
            level.m_sums = create("SurfaceBlockSums", 8, blocks);
            work.m_scanLevels.push_back(level);
            if (blocks == 1)
                break;

            count = blocks;
        }

        return work;
    }


    void EnsureSurfaceWork(SurfaceWork& work, Core::Device* device, const Vector2UInt resolution,
                           const festd::span<const SurfaceBinding> bindings, const uint32_t groupRowCount,
                           const uint32_t shaderCount, const uint32_t waveLaneCount)
    {
        bool compatible = work.m_resolution == resolution && work.m_shaderCount == shaderCount
            && work.m_waveLaneCount == waveLaneCount && work.m_lookupData.size() == Math::Max(groupRowCount, 1u)
            && work.m_sourceBindings.size() == bindings.size();
        if (compatible)
        {
            for (uint32_t index = 0; index < bindings.size(); ++index)
            {
                const SurfaceBinding& previous = work.m_sourceBindings[index];
                compatible = previous.m_group == bindings[index].m_group && previous.m_shaderSlot == bindings[index].m_shaderSlot;
                if (!compatible)
                    break;
            }
        }

        if (compatible)
            return;

        work = CreateSurfaceWork(device, resolution, bindings, groupRowCount, shaderCount, waveLaneCount);
    }


    void ShareSurfaceStorage(Core::Device* device, SurfaceWork& evaluation, SurfaceWork& shading)
    {
        FE_Assert(evaluation.m_resolution == shading.m_resolution);
        const auto share = [&](Rc<Core::Buffer>& first, Rc<Core::Buffer>& second, const uint32_t required) {
            const uint32_t firstSize = first->GetDesc().m_size;
            const uint32_t secondSize = second->GetDesc().m_size;
            if (first->GetDesc().m_size < required)
                first = Core::Buffer::CreateByteAddress(device, first->GetName(), required);

            second = first;
            evaluation.m_scratchBytes += first->GetDesc().m_size - firstSize;
            shading.m_scratchBytes += second->GetDesc().m_size - secondSize;
        };
        share(evaluation.m_pixels,
              shading.m_pixels,
              Math::Max(evaluation.m_pixels->GetDesc().m_size, shading.m_pixels->GetDesc().m_size));
        share(evaluation.m_waves,
              shading.m_waves,
              Math::Max(evaluation.m_waves->GetDesc().m_size, shading.m_waves->GetDesc().m_size));
    }


    void BuildSurfaceWork(Core::FrameGraph& graph, Core::RingUploader& uploader, Core::Buffer* view, Core::Texture* visibility,
                          Core::Texture* g0, SurfaceWork& work, const bool classifyShading)
    {
        FE_Assert(Vector2UInt(g0->GetDesc().m_width, g0->GetDesc().m_height) == work.m_resolution);
        const auto commit = [&](Core::Buffer* buffer) {
            if (buffer->GetMemoryStatus() != Core::ResourceMemory::kNotCommitted)
                return;

            Core::ResourceCommitParams params;
            params.m_memory = Core::ResourceMemory::kDeviceLocal;
            params.m_bindFlags = Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite
                | Core::BarrierAccessFlags::kCopySource | Core::BarrierAccessFlags::kCopyDest;
            if (buffer == work.m_arguments.Get())
                params.m_bindFlags |= Core::BarrierAccessFlags::kIndirectArgument;

            graph.GetResourcePool()->CommitBufferMemory(buffer, params);
        };
        for (Core::Buffer* buffer : { work.m_lookup.Get(),
                                      work.m_bindings.Get(),
                                      work.m_shaderBindings.Get(),
                                      work.m_counts.Get(),
                                      work.m_cursors.Get(),
                                      work.m_pixels.Get(),
                                      work.m_waves.Get(),
                                      work.m_ranges.Get(),
                                      work.m_arguments.Get() })
            commit(buffer);

        for (const SurfaceScanLevel& level : work.m_scanLevels)
        {
            commit(level.m_offsets.Get());
            commit(level.m_sums.Get());
        }

        if (!work.m_uploaded)
        {
            FE_Verify(uploader.UploadArray(graph, work.m_lookup.Get(), festd::span(work.m_lookupData)));
            FE_Verify(uploader.UploadArray(graph, work.m_bindings.Get(), festd::span(work.m_bindingData)));
            FE_Verify(uploader.UploadArray(graph, work.m_shaderBindings.Get(), festd::span(work.m_shaderBindingData)));
            work.m_uploaded = true;
        }

        TilingConstants dimensions{};
        dimensions.m_resolution = work.m_resolution;
        dimensions.m_tilesPerRow = Math::CeilDivide(work.m_resolution.x, kTileSize);
        dimensions.m_tileCount = work.m_tileCount;
        dimensions.m_bucketCount = work.m_bucketCount;
        dimensions.m_groupRowCount = work.m_lookupData.size();
        dimensions.m_shaderCount = work.m_shaderCount;
        dimensions.m_waveLaneCount = work.m_waveLaneCount;
        dimensions.m_classifyShading = classifyShading;
        const uint32_t pixels = work.m_resolution.x * work.m_resolution.y;
        auto* clear = AllocateStep(graph, ClearPipeline::GetPipeline(), dimensions);
        clear->m_constants.m_counts = graph.GetUAV(work.m_counts.Get());
        clear->m_constants.m_cursors = graph.GetUAV(work.m_cursors.Get());
        if (!classifyShading)
            clear->m_constants.m_g0 = graph.GetUAV(Core::TextureView::Create(g0));
        graph.AddDispatchPass("ClearSurfaceWork",
                              clear,
                              DispatchSize(classifyShading ? work.m_bucketCount : Math::Max(pixels, work.m_bucketCount)));
        auto* count = AllocateStep(graph, CountPipeline::GetPipeline(), dimensions);
        if (classifyShading)
            count->m_constants.m_g0Read = graph.GetSRV(Core::TextureView::Create(g0));
        else
        {
            count->m_constants.m_view = graph.GetSRV(view);
            count->m_constants.m_visibility = graph.GetSRV(Core::TextureView::Create(visibility));
            count->m_constants.m_g0 = graph.GetUAV(Core::TextureView::Create(g0));
        }
        count->m_constants.m_lookup = graph.GetSRV(work.m_lookup.Get());
        count->m_constants.m_counts = graph.GetUAV(work.m_counts.Get());
        const Core::ComputeWorkGroupCount screenGroups(
            Vector2UInt(Math::CeilDivide(work.m_resolution.x, 16u), Math::CeilDivide(work.m_resolution.y, 16u)));
        graph.AddDispatchPass("CountSurfacePixels", count, screenGroups);
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

        auto* scatter = AllocateStep(graph, ScatterPipeline::GetPipeline(), dimensions);
        if (classifyShading)
            scatter->m_constants.m_g0Read = graph.GetSRV(Core::TextureView::Create(g0));
        else
        {
            scatter->m_constants.m_view = graph.GetSRV(view);
            scatter->m_constants.m_visibility = graph.GetSRV(Core::TextureView::Create(visibility));
        }
        scatter->m_constants.m_lookup = graph.GetSRV(work.m_lookup.Get());
        scatter->m_constants.m_offsets = graph.GetSRV(work.m_scanLevels.front().m_offsets.Get());
        scatter->m_constants.m_cursors = graph.GetUAV(work.m_cursors.Get());
        scatter->m_constants.m_pixels = graph.GetUAV(work.m_pixels.Get());
        graph.AddDispatchPass("ScatterSurfacePixels", scatter, screenGroups);
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
