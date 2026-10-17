#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Scene/GpuMeshWorkBuilder.h>

#include <Shaders/Passes/MeshPass/ClearMeshWork.h>
#include <Shaders/Passes/MeshPass/CountMeshInstances.h>
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
        FE_COMPUTE_PIPELINE_SET(CountMeshInstancesPipeline, "Shaders/Passes/MeshPass/CountMeshInstances.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(CullMeshInstancesPipeline, "Shaders/Passes/MeshPass/CullMeshInstances.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(FinalizeMeshBucketsPipeline, "Shaders/Passes/MeshPass/FinalizeMeshBuckets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(PropagateMeshBucketOffsetsPipeline, "Shaders/Passes/MeshPass/PropagateMeshBucketOffsets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScanMeshBucketsPipeline, "Shaders/Passes/MeshPass/ScanMeshBuckets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScatterMeshWorkPipeline, "Shaders/Passes/MeshPass/ScatterMeshWork.cs.hlsl");


        struct CountInput final
        {
            Core::Buffer* m_classification;
            Core::Buffer* m_routing;
            uint32_t m_dispatchCount;
            Vector2UInt m_drawTagMask;
        };


        struct CountOutput final
        {
            Rc<Core::Buffer> m_counts;
            Rc<Core::Buffer> m_cursors;
            Rc<Core::Buffer> m_instanceBuckets;
        };


        struct FinalizeInput final
        {
            Core::Buffer* m_counts;
            Core::Buffer* m_offsets;
            Core::Buffer* m_layouts;
            uint32_t m_bucketCount;
            uint32_t m_commandCapacity;
        };


        struct FinalizeOutput final
        {
            Rc<Core::Buffer> m_buckets;
            Rc<Core::Buffer> m_commands;
            Rc<Core::Buffer> m_arguments;
        };


        struct ScatterInput final
        {
            Core::Buffer* m_classification;
            Core::Buffer* m_instanceBuckets;
            Core::Buffer* m_buckets;
            Core::Buffer* m_cursors;
            uint32_t m_dispatchCount;
            uint32_t m_instanceCapacity;
            uint32_t m_workCapacity;
        };


        struct ScatterOutput final
        {
            Rc<Core::Buffer> m_instances;
            Rc<Core::Buffer> m_work;
        };


        void Clear(Core::FrameGraph& graph, Core::Buffer* counts, Core::Buffer* cursors, const uint32_t bucketCount)
        {
            auto* desc = graph.AllocatePassData<MeshPass::ClearMeshWork::PassDesc>();
            desc->m_constants.m_counts = graph.GetUAV(counts);
            desc->m_constants.m_cursors = graph.GetUAV(cursors);
            desc->m_constants.m_bucketCount = bucketCount;
            desc->m_pipeline = ClearMeshWorkPipeline::GetPipeline();
            graph.AddDispatchPass("ClearMeshWork",
                                  desc,
                                  Math::CeilDivide(bucketCount * 2, MeshPass::ClearMeshWork::kThreadCount));
        }


        CountOutput Count(Core::FrameGraph& graph, const CountInput& input, const uint32_t bucketCount)
        {
            Core::Device* device = graph.GetDevice();
            CountOutput output;
            output.m_counts = Core::Buffer::CreateStructured<uint32_t>(device, "MeshBucketCounts", bucketCount * 2);
            output.m_cursors = Core::Buffer::CreateStructured<uint32_t>(device, "MeshBucketCursors", bucketCount * 2);
            output.m_instanceBuckets =
                Core::Buffer::CreateStructured<uint32_t>(device,
                                                         "MeshInstanceBuckets",
                                                         input.m_dispatchCount * MeshPass::kInstancesPerCullGroup);
            Clear(graph, output.m_counts.Get(), output.m_cursors.Get(), bucketCount);

            auto* desc = graph.AllocatePassData<MeshPass::CountMeshInstances::PassDesc>();
            auto& constants = desc->m_constants;
            constants.m_classification = graph.GetSRV(input.m_classification);
            constants.m_routing = graph.GetSRV(input.m_routing);
            constants.m_instanceBuckets = graph.GetUAV(output.m_instanceBuckets.Get());
            constants.m_counts = graph.GetUAV(output.m_counts.Get());
            constants.m_drawTagMask = input.m_drawTagMask;
            desc->m_pipeline = CountMeshInstancesPipeline::GetPipeline();
            graph.AddDispatchPass("CountEligibleMeshInstances", desc, input.m_dispatchCount);
            return output;
        }


        Rc<Core::Buffer> Scan(Core::FrameGraph& graph, Core::Buffer* counts, const uint32_t bucketCount)
        {
            struct ScanLevel final
            {
                Rc<Core::Buffer> m_offsets;
                uint32_t m_count;
            };

            // Scan block sums recursively, then propagate offsets back to the bucket level.
            festd::fixed_vector<ScanLevel, 4> levels;
            Rc<Core::Buffer> input = counts;
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


        FinalizeOutput Finalize(Core::FrameGraph& graph, const FinalizeInput& input)
        {
            Core::Device* device = graph.GetDevice();
            FinalizeOutput output;
            output.m_buckets =
                Core::Buffer::CreateStructured<MeshPass::PipelineBucket>(device, "MeshBuckets", input.m_bucketCount);
            output.m_commands =
                Core::Buffer::CreateStructured<MeshPass::MeshCommandRange>(device, "MeshCommandRanges", input.m_commandCapacity);
            output.m_arguments = Core::Buffer::CreateStructured<MeshPass::MeshDispatchArguments>(device,
                                                                                                 "MeshIndirectArguments",
                                                                                                 input.m_commandCapacity);

            auto* desc = graph.AllocatePassData<MeshPass::FinalizeMeshBuckets::PassDesc>();
            auto& constants = desc->m_constants;
            constants.m_offsets = graph.GetSRV(input.m_offsets);
            constants.m_counts = graph.GetSRV(input.m_counts);
            constants.m_layouts = graph.GetSRV(input.m_layouts);
            constants.m_buckets = graph.GetUAV(output.m_buckets.Get());
            constants.m_commands = graph.GetUAV(output.m_commands.Get());
            constants.m_arguments = graph.GetUAV(output.m_arguments.Get());
            constants.m_bucketCount = input.m_bucketCount;
            desc->m_pipeline = FinalizeMeshBucketsPipeline::GetPipeline();
            graph.AddDispatchPass("FinalizeMeshBuckets",
                                  desc,
                                  Math::CeilDivide(constants.m_bucketCount, MeshPass::FinalizeMeshBuckets::kThreadCount));
            return output;
        }


        ScatterOutput Scatter(Core::FrameGraph& graph, const ScatterInput& input)
        {
            Core::Device* device = graph.GetDevice();
            ScatterOutput output;
            output.m_instances = Core::Buffer::CreateStructured<MeshPass::VisibleInstance>(device,
                                                                                           "VisibleMeshInstances",
                                                                                           input.m_instanceCapacity);
            output.m_work =
                Core::Buffer::CreateStructured<MeshPass::MeshletWorkChunk>(device, "MeshletWorkChunks", input.m_workCapacity);

            auto* desc = graph.AllocatePassData<MeshPass::ScatterMeshWork::PassDesc>();
            auto& constants = desc->m_constants;
            constants.m_classification = graph.GetSRV(input.m_classification);
            constants.m_instanceBuckets = graph.GetSRV(input.m_instanceBuckets);
            constants.m_buckets = graph.GetSRV(input.m_buckets);
            constants.m_cursors = graph.GetUAV(input.m_cursors);
            constants.m_instances = graph.GetUAV(output.m_instances.Get());
            constants.m_work = graph.GetUAV(output.m_work.Get());
            desc->m_pipeline = ScatterMeshWorkPipeline::GetPipeline();
            graph.AddDispatchPass("ScatterMeshletWork", desc, input.m_dispatchCount);
            return output;
        }
    } // namespace


    void CullView(Core::FrameGraph& graph, Core::RingUploader& uploader, festd::span<const MeshPass::CullDispatch> dispatches,
                  MeshPass::MeshViewData viewData, CulledMeshView& result)
    {
        FE_FG_SCOPE(graph, "CullMeshView");
        if (dispatches.empty())
            return;

        Core::Device* device = graph.GetDevice();
        const auto view = Core::Buffer::CreateStructured<MeshPass::MeshViewData>(device, "MeshCullView", 1);
        const auto dispatchBuffer =
            Core::Buffer::CreateStructured<MeshPass::CullDispatch>(device, "MeshCullDispatches", dispatches.size());
        result.m_viewData = viewData;
        result.m_dispatchCount = dispatches.size();
        result.m_classification = Core::Buffer::CreateStructured<MeshPass::InstanceClassification>(
            device,
            "MeshClassification",
            dispatches.size() * MeshPass::kInstancesPerCullGroup);
        graph.AddBufferDependency(view.Get(), viewData.m_scene);
        FE_Verify(uploader.Upload(graph, view.Get(), viewData));
        FE_Verify(uploader.UploadArray(graph, dispatchBuffer.Get(), dispatches));

        auto* desc = graph.AllocatePassData<MeshPass::CullMeshInstances::PassDesc>();
        desc->m_constants.m_view = graph.GetSRV(view.Get());
        desc->m_constants.m_dispatches = graph.GetSRV(dispatchBuffer.Get());
        desc->m_constants.m_classification = graph.GetUAV(result.m_classification.Get());
        desc->m_pipeline = CullMeshInstancesPipeline::GetPipeline();
        graph.AddDispatchPass("CullMeshInstances", desc, result.m_dispatchCount);
    }


    void Build(Core::FrameGraph& graph, Core::RingUploader& uploader, const MeshPipelineRegistry& registry,
               const CulledMeshView& culled, const Vector2UInt drawTagMask, PreparedMeshPass& result)
    {
        FE_FG_SCOPE(graph, "PrepareMeshPass");
        if (registry.m_slots.empty() || culled.m_dispatchCount == 0)
            return;

        if (registry.m_workCapacity == 0)
            return;

        // Each stage owns its outputs and names only the inputs it consumes.
        const CountInput countInput{ culled.m_classification.Get(),
                                     registry.m_routing.Get(),
                                     culled.m_dispatchCount,
                                     drawTagMask };
        const CountOutput counts = Count(graph, countInput, registry.m_slots.size());
        const auto offsets = Scan(graph, counts.m_counts.Get(), registry.m_slots.size());
        const FinalizeInput finalizeInput{ counts.m_counts.Get(),
                                           offsets.Get(),
                                           registry.m_layouts.Get(),
                                           registry.m_slots.size(),
                                           registry.m_commandCapacity };
        const FinalizeOutput commands = Finalize(graph, finalizeInput);
        const ScatterInput scatterInput{ culled.m_classification.Get(), counts.m_instanceBuckets.Get(),
                                         commands.m_buckets.Get(),      counts.m_cursors.Get(),
                                         culled.m_dispatchCount,        registry.m_instanceCapacity,
                                         registry.m_workCapacity };
        const ScatterOutput scattered = Scatter(graph, scatterInput);

        result.m_instances = scattered.m_instances;
        result.m_work = scattered.m_work;
        result.m_commands = commands.m_commands;
        result.m_arguments = commands.m_arguments;
        result.m_submissions = registry.m_submissions;
        result.m_view = Core::Buffer::CreateStructured<MeshPass::MeshViewData>(graph.GetDevice(), "MeshView", 1);
        MeshPass::MeshViewData viewData = culled.m_viewData;
        viewData.m_visibleInstances = graph.GetSRV(result.m_instances.Get());
        viewData.m_workChunks = graph.GetSRV(result.m_work.Get());
        viewData.m_commands = graph.GetSRV(result.m_commands.Get());
        graph.AddBufferDependency(result.m_view.Get(), viewData.m_scene);
        graph.AddBufferDependency(result.m_view.Get(), result.m_instances.Get());
        graph.AddBufferDependency(result.m_view.Get(), result.m_work.Get());
        graph.AddBufferDependency(result.m_view.Get(), result.m_commands.Get());
        FE_Verify(uploader.Upload(graph, result.m_view.Get(), viewData));
    }
} // namespace FE::Graphics::GpuMeshWorkBuilder
