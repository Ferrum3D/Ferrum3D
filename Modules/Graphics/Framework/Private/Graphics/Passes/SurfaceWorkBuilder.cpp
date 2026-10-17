#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Passes/SurfaceWorkBuilder.h>

namespace FE::Graphics::Visibility
{
    namespace
    {
        FE_COMPUTE_PIPELINE_SET(ClearPipeline, "Shaders/Deferred/ClearSurfaceWork.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(CountPipeline, "Shaders/Deferred/CountSurfacePixels.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScanPipeline, "Shaders/Deferred/ScanSurfaceBuckets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(PropagatePipeline, "Shaders/Deferred/PropagateSurfaceOffsets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScatterPipeline, "Shaders/Deferred/ScatterSurfacePixels.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(CommandsPipeline, "Shaders/Deferred/EmitSurfaceCommands.cs.hlsl");


        uint32_t DispatchSize(const uint32_t threads)
        {
            return Math::Max(Math::CeilDivide(threads, kSurfaceBlockSize), 1u);
        }
    } // namespace


    SurfaceRouting CreateSurfaceRouting(Core::Device* device, const festd::span<const SurfaceBinding> bindings,
                                        const uint32_t groupRowCount, const uint32_t shaderCount)
    {
        SurfaceRouting routing;
        routing.m_bindingCount = bindings.size();
        routing.m_shaderCount = shaderCount;
        routing.m_lookupData.resize(Math::Max(groupRowCount, 1u), SurfaceRoute{ kInvalidIndex, 0, 0 });
        routing.m_shaderBindingData.resize(Math::Max(shaderCount, 1u), Vector2UInt(0, 0));

        festd::vector<SurfaceBinding> sorted;
        sorted.assign(bindings.begin(), bindings.end());
        festd::sort(sorted, [](const SurfaceBinding& lhs, const SurfaceBinding& rhs) {
            return lhs.m_shaderSlot != rhs.m_shaderSlot ? lhs.m_shaderSlot < rhs.m_shaderSlot : lhs.m_group < rhs.m_group;
        });

        routing.m_bindingData.resize(Math::Max(sorted.size(), 1u), kInvalidIndex);

        // Bucket order is shader, tile, then binding, keeping each shader's commands contiguous.
        uint32_t firstBinding = 0;
        for (uint32_t slot = 0; slot < shaderCount; ++slot)
        {
            uint32_t end = firstBinding;
            while (end < sorted.size() && sorted[end].m_shaderSlot == slot)
                ++end;

            const uint32_t count = end - firstBinding;
            routing.m_shaderBindingData[slot] = Vector2UInt(firstBinding, count);
            for (uint32_t index = firstBinding; index < end; ++index)
            {
                const uint32_t group = sorted[index].m_group;
                FE_Assert(group < groupRowCount && routing.m_lookupData[group].m_firstBinding == kInvalidIndex);
                routing.m_lookupData[group] = SurfaceRoute{ firstBinding, index - firstBinding, count };
                routing.m_bindingData[index] = group;
            }

            firstBinding = end;
        }

        FE_Assert(firstBinding == sorted.size(), "Surface binding has an invalid shader slot");
        routing.m_lookup =
            Core::Buffer::CreateByteAddress(device, "SurfaceBindingLookup", sizeof(SurfaceRoute) * routing.m_lookupData.size());
        routing.m_bindings = Core::Buffer::CreateByteAddress(device, "SurfaceBindings", 4 * routing.m_bindingData.size());
        routing.m_shaderBindings =
            Core::Buffer::CreateByteAddress(device, "SurfaceShaderBindings", 8 * routing.m_shaderBindingData.size());
        return routing;
    }


    SurfaceWork CreateSurfaceWork(Core::Device* device, const Vector2UInt resolution, SurfaceRouting& routing,
                                  const uint32_t waveLaneCount)
    {
        FE_Assert(waveLaneCount == 32 || waveLaneCount == 64);
        FE_Assert(resolution.x > 0 && resolution.y > 0);

        SurfaceWork work;
        work.m_routing = &routing;
        work.m_resolution = resolution;
        work.m_shaderCount = routing.m_shaderCount;
        work.m_waveLaneCount = waveLaneCount;
        const Vector2UInt tileCount = Math::CeilDivide(resolution, kTileSize);
        const uint32_t tiles = tileCount.x * tileCount.y;
        const uint32_t pixels = tiles * kTileSize * kTileSize;
        work.m_tileCount = tiles;
        work.m_bucketCount = tiles * routing.m_bindingCount;
        // Every nonempty bucket may contribute one partial wave.
        work.m_waveCapacity = routing.m_bindingCount == 0
            ? 0
            : Math::Min(pixels, Math::CeilDivide(pixels, waveLaneCount) + Math::Min(pixels, work.m_bucketCount));
        work.m_counts = Core::Buffer::CreateByteAddress(device, "SurfaceCounts", 4 * Math::Max(work.m_bucketCount, 1u));
        work.m_cursors = Core::Buffer::CreateByteAddress(device, "SurfaceCursors", 4 * Math::Max(work.m_bucketCount, 1u));
        work.m_pixels = Core::Buffer::CreateByteAddress(device,
                                                        "SurfacePixels",
                                                        4 * Math::Max(routing.m_bindingCount == 0 ? 0u : pixels, 1u));
        work.m_waves = Core::Buffer::CreateByteAddress(device, "SurfaceWaves", 16 * Math::Max(work.m_waveCapacity, 1u));
        work.m_ranges = Core::Buffer::CreateByteAddress(device, "SurfaceRanges", 8 * Math::Max(work.m_shaderCount, 1u));
        work.m_arguments = Core::Buffer::CreateByteAddress(device, "SurfaceArguments", 12 * Math::Max(work.m_shaderCount, 1u));
        work.m_emissionArguments = Core::Buffer::CreateByteAddress(device, "SurfaceEmissionArguments", sizeof(DispatchArguments));

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


    namespace
    {
        void UploadRouting(Core::FrameGraph& graph, Core::RingUploader& uploader, SurfaceRouting& routing)
        {
            if (routing.m_uploaded)
                return;

            FE_Verify(uploader.UploadArray(graph, routing.m_lookup.Get(), festd::span(routing.m_lookupData)));
            FE_Verify(uploader.UploadArray(graph, routing.m_bindings.Get(), festd::span(routing.m_bindingData)));
            FE_Verify(uploader.UploadArray(graph, routing.m_shaderBindings.Get(), festd::span(routing.m_shaderBindingData)));
            routing.m_uploaded = true;
        }


        void Clear(Core::FrameGraph& graph, const SurfaceWork& work)
        {
            auto* clear = graph.AllocatePassData<ClearSurfacePassDesc>();
            clear->m_pipeline = ClearPipeline::GetPipeline();
            clear->m_constants.m_counts = graph.GetUAV(work.m_counts.Get());
            clear->m_constants.m_cursors = graph.GetUAV(work.m_cursors.Get());
            clear->m_constants.m_bucketCount = work.m_bucketCount;
            graph.AddDispatchPass("ClearSurfaceWork", clear, DispatchSize(work.m_bucketCount));
        }


        // Prefix sums carry both pixel counts and wave counts through the same hierarchy.
        void Scan(Core::FrameGraph& graph, const SurfaceWork& work)
        {
            for (uint32_t index = 0; index < work.m_scanLevels.size(); ++index)
            {
                const SurfaceScanLevel& level = work.m_scanLevels[index];
                auto* scan = graph.AllocatePassData<SurfaceScanPassDesc>();
                scan->m_pipeline = ScanPipeline::GetPipeline();
                scan->m_constants.m_scanCount = level.m_count;
                scan->m_constants.m_scanBuckets = index == 0;
                scan->m_constants.m_waveLaneCount = work.m_waveLaneCount;
                if (index + 1 == work.m_scanLevels.size())
                {
                    scan->m_constants.m_emissionArguments = graph.GetUAV(work.m_emissionArguments.Get());
                    scan->m_constants.m_shaderCount = work.m_shaderCount;
                }

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
                auto* propagate = graph.AllocatePassData<SurfacePropagatePassDesc>();
                propagate->m_pipeline = PropagatePipeline::GetPipeline();
                propagate->m_constants.m_scanCount = child.m_count;
                propagate->m_constants.m_scanOutput = graph.GetUAV(child.m_offsets.Get());
                propagate->m_constants.m_parentOffsets = graph.GetSRV(work.m_scanLevels[index].m_offsets.Get());
                graph.AddDispatchPass("PropagateSurfaceOffsets", propagate, DispatchSize(child.m_count));
            }
        }


        // Divide each material run into waves and emit an indirect dispatch per registered shader.
        void EmitCommands(Core::FrameGraph& graph, const SurfaceWork& work)
        {
            const SurfaceRouting& routing = *work.m_routing;
            auto* commands = graph.AllocatePassData<SurfaceCommandsPassDesc>();
            commands->m_pipeline = CommandsPipeline::GetPipeline();
            commands->m_arguments = Core::BufferView(work.m_emissionArguments.Get());
            commands->m_constants.m_offsets = graph.GetSRV(work.m_scanLevels.front().m_offsets.Get());
            commands->m_constants.m_totals = graph.GetSRV(work.m_scanLevels.back().m_sums.Get());
            commands->m_constants.m_countsRead = graph.GetSRV(work.m_counts.Get());
            commands->m_constants.m_bindings = graph.GetSRV(routing.m_bindings.Get());
            commands->m_constants.m_shaderBindings = graph.GetSRV(routing.m_shaderBindings.Get());
            commands->m_constants.m_waves = graph.GetUAV(work.m_waves.Get());
            commands->m_constants.m_ranges = graph.GetUAV(work.m_ranges.Get());
            commands->m_constants.m_arguments = graph.GetUAV(work.m_arguments.Get());
            commands->m_constants.m_tileCount = work.m_tileCount;
            commands->m_constants.m_bucketCount = work.m_bucketCount;
            commands->m_constants.m_shaderCount = work.m_shaderCount;
            commands->m_constants.m_waveLaneCount = work.m_waveLaneCount;
            graph.AddDispatchIndirectPass("EmitSurfaceCommands", commands);
        }


        template<class Constants>
        void SetClassificationGrid(Constants& constants, const SurfaceWork& work)
        {
            constants.m_resolution = work.m_resolution;
            constants.m_tilesPerRow = Math::CeilDivide(work.m_resolution.x, kTileSize);
            constants.m_tileCount = work.m_tileCount;
            constants.m_groupRowCount = work.m_routing->m_lookupData.size();
        }
    } // namespace


    void BuildSurfaceWork(Core::FrameGraph& graph, Core::RingUploader& uploader, const EvaluationWorkInput& input,
                          SurfaceWork& work)
    {
        FE_Assert(input.m_objectData && input.m_countPipeline && input.m_scatterPipeline);
        FE_Assert(input.m_visibility->GetDesc().GetSize2D() == work.m_resolution);
        UploadRouting(graph, uploader, *work.m_routing);
        Clear(graph, work);

        // Object producers supply visibility classification; no fallback can interpret their private payloads.
        auto* count = graph.AllocatePassData<EvaluationCountPassDesc>();
        count->m_pipeline = input.m_countPipeline;
        SetClassificationGrid(count->m_constants, work);
        count->m_constants.m_view = graph.GetSRV(input.m_objectData);
        count->m_constants.m_visibility = graph.GetSRV(input.m_visibility);
        count->m_constants.m_lookup = graph.GetSRV(work.m_routing->m_lookup.Get());
        count->m_constants.m_counts = graph.GetUAV(work.m_counts.Get());
        const Core::ComputeWorkGroupCount screenGroups = Math::CeilDivide(work.m_resolution, 16u);
        graph.AddDispatchPass("CountSurfacePixels", count, screenGroups);

        Scan(graph, work);

        auto* scatter = graph.AllocatePassData<EvaluationScatterPassDesc>();
        scatter->m_pipeline = input.m_scatterPipeline;
        SetClassificationGrid(scatter->m_constants, work);
        scatter->m_constants.m_view = graph.GetSRV(input.m_objectData);
        scatter->m_constants.m_visibility = graph.GetSRV(input.m_visibility);
        scatter->m_constants.m_lookup = graph.GetSRV(work.m_routing->m_lookup.Get());
        scatter->m_constants.m_offsets = graph.GetSRV(work.m_scanLevels.front().m_offsets.Get());
        scatter->m_constants.m_cursors = graph.GetUAV(work.m_cursors.Get());
        scatter->m_constants.m_pixels = graph.GetUAV(work.m_pixels.Get());
        graph.AddDispatchPass("ScatterSurfacePixels", scatter, screenGroups);

        EmitCommands(graph, work);
    }


    void BuildSurfaceWork(Core::FrameGraph& graph, Core::RingUploader& uploader, const ShadingWorkInput& input, SurfaceWork& work)
    {
        FE_Assert(input.m_g0->GetDesc().GetSize2D() == work.m_resolution);
        UploadRouting(graph, uploader, *work.m_routing);
        Clear(graph, work);

        auto* count = graph.AllocatePassData<ShadingCountPassDesc>();
        count->m_pipeline = CountPipeline::GetPipeline();
        SetClassificationGrid(count->m_constants, work);
        count->m_constants.m_g0Read = graph.GetSRV(input.m_g0);
        count->m_constants.m_lookup = graph.GetSRV(work.m_routing->m_lookup.Get());
        count->m_constants.m_counts = graph.GetUAV(work.m_counts.Get());
        const Core::ComputeWorkGroupCount screenGroups = Math::CeilDivide(work.m_resolution, 16u);
        graph.AddDispatchPass("CountSurfacePixels", count, screenGroups);

        Scan(graph, work);

        auto* scatter = graph.AllocatePassData<ShadingScatterPassDesc>();
        scatter->m_pipeline = ScatterPipeline::GetPipeline();
        SetClassificationGrid(scatter->m_constants, work);
        scatter->m_constants.m_g0Read = graph.GetSRV(input.m_g0);
        scatter->m_constants.m_lookup = graph.GetSRV(work.m_routing->m_lookup.Get());
        scatter->m_constants.m_offsets = graph.GetSRV(work.m_scanLevels.front().m_offsets.Get());
        scatter->m_constants.m_cursors = graph.GetUAV(work.m_cursors.Get());
        scatter->m_constants.m_pixels = graph.GetUAV(work.m_pixels.Get());
        graph.AddDispatchPass("ScatterSurfacePixels", scatter, screenGroups);

        EmitCommands(graph, work);
    }
} // namespace FE::Graphics::Visibility
