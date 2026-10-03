#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Scene/GpuMeshWorkBuilder.h>

#include <Shaders/Passes/MeshPass/ClearMeshWork.h>
#include <Shaders/Passes/MeshPass/CullMeshInstances.h>
#include <Shaders/Passes/MeshPass/FinalizeMeshBuckets.h>
#include <Shaders/Passes/MeshPass/PropagateMeshBucketOffsets.h>
#include <Shaders/Passes/MeshPass/ScanMeshBuckets.h>
#include <Shaders/Passes/MeshPass/ScatterMeshWork.h>

namespace FE::Graphics::GpuMeshWorkBuilder
{
    namespace
    {
        FE_COMPUTE_PIPELINE_SET(ClearMeshWorkPipeline, "Shaders/Passes/MeshPass/ClearMeshWork.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(CullMeshInstancesPipeline, "Shaders/Passes/MeshPass/CullMeshInstances.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(FinalizeMeshBucketsPipeline, "Shaders/Passes/MeshPass/FinalizeMeshBuckets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(PropagateMeshBucketOffsetsPipeline, "Shaders/Passes/MeshPass/PropagateMeshBucketOffsets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScanMeshBucketsPipeline, "Shaders/Passes/MeshPass/ScanMeshBuckets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScatterMeshWorkPipeline, "Shaders/Passes/MeshPass/ScatterMeshWork.cs.hlsl");


        struct PreparationData final
        {
            PreparedMeshView m_output;
            Rc<Core::Buffer> m_dispatches;
            Rc<Core::Buffer> m_classification;
            Rc<Core::Buffer> m_routing;
            Rc<Core::Buffer> m_layouts;
            Rc<Core::Buffer> m_counts;
            Rc<Core::Buffer> m_cursors;
            Rc<Core::Buffer> m_buckets;
            uint32_t m_dispatchCount = 0;
        };


        void Clear(Core::FrameGraph& graph, const PreparationData& work, uint32_t bucketCount)
        {
            auto* desc = graph.AllocatePassData<MeshPass::ClearMeshWork::PassDesc>();
            auto& constants = desc->m_constants;
            constants.m_counts = graph.GetUAV(work.m_counts.Get());
            constants.m_cursors = graph.GetUAV(work.m_cursors.Get());
            constants.m_bucketCount = bucketCount;

            desc->m_pipeline = ClearMeshWorkPipeline::GetPipeline();
            graph.AddDispatchPass("ClearMeshWork",
                                  desc,
                                  Math::CeilDivide(bucketCount * 2, MeshPass::ClearMeshWork::kThreadCount));
        }


        void Cull(Core::FrameGraph& graph, const PreparationData& work)
        {
            auto* desc = graph.AllocatePassData<MeshPass::CullMeshInstances::PassDesc>();
            auto& constants = desc->m_constants;
            constants.m_view = graph.GetSRV(work.m_output.m_view.Get());
            constants.m_dispatches = graph.GetSRV(work.m_dispatches.Get());
            constants.m_classification = graph.GetUAV(work.m_classification.Get());
            constants.m_counts = graph.GetUAV(work.m_counts.Get());
            desc->m_routing = { work.m_routing.Get(),
                                Core::BarrierSyncFlags::kComputeShading,
                                Core::BarrierAccessFlags::kShaderRead };

            desc->m_pipeline = CullMeshInstancesPipeline::GetPipeline();
            graph.AddDispatchPass("CullAndCountMeshInstances", desc, work.m_dispatchCount);
        }


        Rc<Core::Buffer> Scan(Core::FrameGraph& graph, PreparationData& work, uint32_t bucketCount)
        {
            struct ScanLevel final
            {
                Rc<Core::Buffer> m_offsets;
                uint32_t m_count;
            };

            // Scan block sums recursively, then propagate offsets back to the bucket level.
            festd::fixed_vector<ScanLevel, 4> levels;
            Rc<Core::Buffer> input = work.m_counts;
            uint32_t count = bucketCount;
            for (;;)
            {
                const uint32_t blocks = Math::CeilDivide(count, MeshPass::ScanMeshBuckets::kThreadCount);
                const auto offsets = Core::Buffer::CreateStructured<Vector2UInt>(graph.GetDevice(), "MeshBucketOffsets", count);
                const auto sums = Core::Buffer::CreateStructured<Vector2UInt>(graph.GetDevice(), "MeshBucketBlockSums", blocks);

                auto* desc = graph.AllocatePassData<MeshPass::ScanMeshBuckets::PassDesc>();
                auto& constants = desc->m_constants;
                constants.m_input = graph.GetSRV(input.Get());
                constants.m_output = graph.GetUAV(offsets.Get());
                constants.m_sums = graph.GetUAV(sums.Get());
                constants.m_count = count;
                desc->m_pipeline = ScanMeshBucketsPipeline::GetPipeline();
                graph.AddDispatchPass("ScanMeshBucketBlocks", desc, blocks);

                levels.push_back({ offsets, count });
                if (blocks == 1)
                    break;

                input = sums;
                count = blocks;
            }

            for (uint32_t level = levels.size() - 1; level > 0; --level)
            {
                auto* desc = graph.AllocatePassData<MeshPass::PropagateMeshBucketOffsets::PassDesc>();
                auto& constants = desc->m_constants;
                constants.m_input = graph.GetSRV(levels[level].m_offsets.Get());
                constants.m_output = graph.GetUAV(levels[level - 1].m_offsets.Get());
                constants.m_count = levels[level - 1].m_count;
                desc->m_pipeline = PropagateMeshBucketOffsetsPipeline::GetPipeline();
                graph.AddDispatchPass("PropagateMeshBucketOffsets",
                                      desc,
                                      Math::CeilDivide(constants.m_count, MeshPass::PropagateMeshBucketOffsets::kThreadCount));
            }

            return levels.front().m_offsets;
        }


        void Finalize(Core::FrameGraph& graph, const PreparationData& work, Core::Buffer* offsets, uint32_t bucketCount)
        {
            auto* desc = graph.AllocatePassData<MeshPass::FinalizeMeshBuckets::PassDesc>();
            auto& constants = desc->m_constants;
            constants.m_offsets = graph.GetSRV(offsets);
            constants.m_counts = graph.GetSRV(work.m_counts.Get());
            constants.m_layouts = graph.GetSRV(work.m_layouts.Get());
            constants.m_buckets = graph.GetUAV(work.m_buckets.Get());
            constants.m_commands = graph.GetUAV(work.m_output.m_commands.Get());
            constants.m_arguments = graph.GetUAV(work.m_output.m_arguments.Get());
            constants.m_bucketCount = bucketCount;

            desc->m_pipeline = FinalizeMeshBucketsPipeline::GetPipeline();
            graph.AddDispatchPass("FinalizeMeshBuckets",
                                  desc,
                                  Math::CeilDivide(bucketCount, MeshPass::FinalizeMeshBuckets::kThreadCount));
        }


        void Scatter(Core::FrameGraph& graph, const PreparationData& work)
        {
            auto* desc = graph.AllocatePassData<MeshPass::ScatterMeshWork::PassDesc>();
            auto& constants = desc->m_constants;
            constants.m_classification = graph.GetSRV(work.m_classification.Get());
            constants.m_buckets = graph.GetSRV(work.m_buckets.Get());
            constants.m_cursors = graph.GetUAV(work.m_cursors.Get());
            constants.m_instances = graph.GetUAV(work.m_output.m_instances.Get());
            constants.m_work = graph.GetUAV(work.m_output.m_work.Get());

            desc->m_pipeline = ScatterMeshWorkPipeline::GetPipeline();
            graph.AddDispatchPass("ScatterMeshletWork", desc, work.m_dispatchCount);
        }
    } // namespace


    void Build(Core::FrameGraph& graph, Core::RingUploader& uploader, const MeshPipelineRegistry& registry,
               festd::span<const MeshPass::CullDispatch> dispatches, MeshPass::ViewData viewData, PreparedMeshView& result)
    {
        FE_FG_SCOPE(graph, "PrepareMeshView");
        if (registry.m_slots.empty() || dispatches.empty())
            return;

        Core::Device* device = graph.GetDevice();
        PreparationData work;

        // Reserve disjoint command pages for each registry slot, including inactive slots with zero pages.
        uint32_t instanceCapacity = 0;
        uint32_t workCapacity = 0;
        uint32_t commandCapacity = 0;
        festd::pmr::vector<MeshPass::BucketLayout> layouts{ graph.GetAllocator() };
        layouts.reserve(registry.m_slots.size());

        for (const MeshPipelineSlot& slot : registry.m_slots)
        {
            const uint32_t pageCount = Math::CeilDivide(slot.m_workCapacity, MeshPass::kWorkChunksPerCommand);

            layouts.push_back({ commandCapacity, pageCount });
            if (pageCount != 0)
                work.m_output.m_submissions.push_back({ slot.m_pipeline, slot.m_pass, commandCapacity, pageCount });

            instanceCapacity += slot.m_instanceCapacity;
            workCapacity += slot.m_workCapacity;
            commandCapacity += pageCount;
        }

        if (workCapacity == 0)
            return;

        // Allocate per-view outputs and upload only view data, culling dispatch entries, and page layouts.
        work.m_output.m_instances =
            Core::Buffer::CreateStructured<MeshPass::VisibleInstance>(device, "VisibleMeshInstances", instanceCapacity);
        work.m_output.m_work =
            Core::Buffer::CreateStructured<MeshPass::MeshletWorkChunk>(device, "MeshletWorkChunks", workCapacity);
        work.m_output.m_commands =
            Core::Buffer::CreateStructured<MeshPass::MeshCommandRange>(device, "MeshCommandRanges", commandCapacity);
        work.m_output.m_arguments =
            Core::Buffer::CreateStructured<MeshPass::MeshDispatchArguments>(device, "MeshIndirectArguments", commandCapacity);
        work.m_output.m_view = Core::Buffer::CreateStructured<MeshPass::ViewData>(device, "MeshView", 1);
        work.m_dispatches =
            Core::Buffer::CreateStructured<MeshPass::CullDispatch>(device, "MeshCullDispatches", dispatches.size());
        work.m_layouts = Core::Buffer::CreateStructured<MeshPass::BucketLayout>(device, "MeshBucketLayouts", layouts.size());
        work.m_classification = Core::Buffer::CreateStructured<MeshPass::InstanceClassification>(
            device,
            "MeshClassification",
            dispatches.size() * MeshPass::kInstancesPerCullGroup);
        work.m_counts = Core::Buffer::CreateStructured<uint32_t>(device, "MeshBucketCounts", layouts.size() * 2);
        work.m_cursors = Core::Buffer::CreateStructured<uint32_t>(device, "MeshBucketCursors", layouts.size() * 2);
        work.m_buckets = Core::Buffer::CreateStructured<MeshPass::PipelineBucket>(device, "MeshBuckets", layouts.size());
        work.m_routing = registry.m_routing;
        work.m_dispatchCount = dispatches.size();

        viewData.m_routing = graph.GetSRV(registry.m_routing.Get());
        viewData.m_visibleInstances = graph.GetSRV(work.m_output.m_instances.Get());
        viewData.m_workChunks = graph.GetSRV(work.m_output.m_work.Get());
        viewData.m_commands = graph.GetSRV(work.m_output.m_commands.Get());
        FE_Verify(uploader.Upload(graph, work.m_output.m_view.Get(), viewData));
        FE_Verify(uploader.UploadArray(graph, work.m_layouts.Get(), festd::span(layouts)));
        FE_Verify(uploader.UploadArray(graph, work.m_dispatches.Get(), dispatches));

        // Count visible work, scan bucket ranges, construct all command pages, then scatter meshlet chunks.
        Clear(graph, work, layouts.size());
        Cull(graph, work);
        const auto offsets = Scan(graph, work, layouts.size());
        Finalize(graph, work, offsets.Get(), layouts.size());
        Scatter(graph, work);
        result = std::move(work.m_output);
    }
} // namespace FE::Graphics::GpuMeshWorkBuilder
