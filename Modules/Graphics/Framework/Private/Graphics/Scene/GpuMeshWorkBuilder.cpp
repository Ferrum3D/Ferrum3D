#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/PipelineVariantSet.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Scene/GpuMeshWorkBuilder.h>

#include <Shaders/Passes/MeshPass/ClearMeshWork.h>
#include <Shaders/Passes/MeshPass/CountMeshInstances.h>
#include <Shaders/Passes/MeshPass/CullMeshInstances.h>
#include <Shaders/Passes/MeshPass/CullMeshlets.h>
#include <Shaders/Passes/MeshPass/FinalizeMeshBuckets.h>
#include <Shaders/Passes/MeshPass/FinalizeMeshletCommands.h>
#include <Shaders/Passes/MeshPass/OcclusionCullMeshInstances.h>
#include <Shaders/Passes/MeshPass/OcclusionCullMeshlets.h>
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
        FE_COMPUTE_PIPELINE_SET(CullMeshletsPipeline, "Shaders/Passes/MeshPass/CullMeshlets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(FinalizeMeshBucketsPipeline, "Shaders/Passes/MeshPass/FinalizeMeshBuckets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(FinalizeMeshletCommandsPipeline, "Shaders/Passes/MeshPass/FinalizeMeshletCommands.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(OcclusionCullMeshInstancesPipeline, "Shaders/Passes/MeshPass/OcclusionCullMeshInstances.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(OcclusionCullMeshletsPipeline, "Shaders/Passes/MeshPass/OcclusionCullMeshlets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(PropagateMeshBucketOffsetsPipeline, "Shaders/Passes/MeshPass/PropagateMeshBucketOffsets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScanMeshBucketsPipeline, "Shaders/Passes/MeshPass/ScanMeshBuckets.cs.hlsl");
        FE_COMPUTE_PIPELINE_SET(ScatterMeshWorkPipeline, "Shaders/Passes/MeshPass/ScatterMeshWork.cs.hlsl");


        struct CountInput final
        {
            Core::StructuredBufferPtr<MeshPass::InstanceClassification> m_classification;
            Core::StructuredBufferPtr<MeshPass::PipelineRouting> m_routing;
            uint32_t m_dispatchCount;
            Vector2UInt m_drawTagMask;
        };


        struct CountOutput final
        {
            Core::StructuredBuffer<uint32_t> m_counts;
            Core::StructuredBuffer<uint32_t> m_cursors;
            Core::StructuredBuffer<uint32_t> m_meshletCounts;
            Core::StructuredBuffer<uint32_t> m_instanceBuckets;
        };


        struct FinalizeOutput final
        {
            Core::StructuredBuffer<MeshPass::PipelineBucket> m_buckets;
            Core::StructuredBuffer<MeshPass::MeshletCullDispatch> m_cullDispatch;
        };


        struct ScatterInput final
        {
            Core::StructuredBufferPtr<MeshPass::InstanceClassification> m_classification;
            Core::StructuredBufferPtr<uint32_t> m_instanceBuckets;
            Core::StructuredBufferPtr<MeshPass::PipelineBucket> m_buckets;
            Core::StructuredBufferPtr<uint32_t> m_cursors;
            Core::StructuredBufferPtr<uint32_t> m_stats;
            uint32_t m_dispatchCount;
            uint32_t m_instanceCapacity;
            uint32_t m_workCapacity;
            uint32_t m_phase;
        };


        struct ScatterOutput final
        {
            Core::StructuredBuffer<MeshPass::VisibleInstance> m_instances;
            Core::StructuredBuffer<MeshPass::MeshletWorkChunk> m_work;
        };


        struct MeshletCullInput final
        {
            ConstantsPtr<MeshPass::MeshViewData> m_view;
            Core::StructuredBufferPtr<MeshPass::VisibleInstance> m_instances;
            Core::StructuredBufferPtr<MeshPass::MeshletWorkChunk> m_work;
            Core::StructuredBufferPtr<MeshPass::PipelineBucket> m_buckets;
            Core::StructuredBufferPtr<MeshPass::MeshletCullDispatch> m_cullDispatch;
            Core::StructuredBufferPtr<uint32_t> m_meshletCounts;
            Core::StructuredBufferPtr<uint32_t> m_meshletVisibility;
            Core::StructuredBufferPtr<uint32_t> m_stats;
            const Tools::DepthPyramid::Output* m_depthPyramid;
            uint32_t m_meshletVisibilityCapacity;
            uint32_t m_meshletCapacity;
            uint32_t m_phase;
        };


        //! Survivors of frustum culling that an occlusion pass tests, with the indirect arguments that cover them.
        template<class TCandidate>
        struct OcclusionCandidates final
        {
            Core::StructuredBuffer<TCandidate> m_candidates;
            Core::StructuredBuffer<MeshPass::OcclusionCullDispatch> m_dispatch;
        };


        struct CommandInput final
        {
            Core::StructuredBufferPtr<MeshPass::PipelineBucket> m_buckets;
            Core::StructuredBufferPtr<uint32_t> m_meshletCounts;
            Core::StructuredBufferPtr<MeshPass::BucketLayout> m_layouts;
            uint32_t m_bucketCount;
            uint32_t m_commandCapacity;
        };


        struct CommandOutput final
        {
            Core::StructuredBuffer<MeshPass::MeshCommandRange> m_commands;
            Core::StructuredBuffer<MeshPass::MeshDispatchArguments> m_arguments;
        };


        MeshPass::DepthPyramidBinding CreateDepthPyramidBinding(Core::FrameGraph& graph,
                                                                const Tools::DepthPyramid::Output& pyramid)
        {
            MeshPass::DepthPyramidBinding binding{};
            binding.m_texture = graph.GetSRV(pyramid.m_texture.Get());
            binding.m_size = pyramid.m_size;
            binding.m_uvScale = pyramid.m_uvScale;
            binding.m_mipCount = pyramid.m_mipCount;
            return binding;
        }


        template<class T>
        RWStructuredBufferDescriptor<T> GetOptionalUAV(Core::FrameGraph& graph, const Core::StructuredBufferPtr<T> buffer)
        {
            return buffer ? graph.GetUAV(buffer) : RWStructuredBufferDescriptor<T>{};
        }


        template<class TCandidate>
        OcclusionCandidates<TCandidate> CreateOcclusionCandidates(Core::FrameGraph& graph, Core::RingUploader& uploader,
                                                                  const Env::Name name, const uint32_t capacity)
        {
            OcclusionCandidates<TCandidate> result;
            result.m_candidates = Core::Buffer::CreateStructured<TCandidate>(graph.GetDevice(), name, capacity);
            result.m_dispatch =
                Core::Buffer::CreateStructured<MeshPass::OcclusionCullDispatch>(graph.GetDevice(), "OcclusionCullDispatch", 1);
            FE_Verify(uploader.Upload(graph, result.m_dispatch.Get(), MeshPass::OcclusionCullDispatch{ 0, 1, 1, 0 }));
            return result;
        }


        template<class TCandidate>
        Core::BufferView GetOcclusionArguments(const OcclusionCandidates<TCandidate>& candidates)
        {
            return Core::BufferView(candidates.m_dispatch.Get()).Slice(0, sizeof(MeshPass::MeshDispatchArguments));
        }


        void OcclusionCullInstances(Core::FrameGraph& graph, const CulledMeshView& culled, const uint32_t phase,
                                    const Tools::DepthPyramid::Output& depthPyramid,
                                    const Core::StructuredBufferPtr<MeshPass::InstanceClassification> classification,
                                    const OcclusionCandidates<uint32_t>& candidates)
        {
            MeshPass::OcclusionCullMeshInstances::Constants constants{};
            constants.m_view = culled.m_view;
            constants.m_candidates = graph.GetSRV(candidates.m_candidates.Get());
            constants.m_dispatch = graph.GetSRV(candidates.m_dispatch.Get());
            constants.m_classification = graph.GetUAV(classification);
            constants.m_visibility = graph.GetUAV(culled.m_visibility.Get());
            constants.m_stats = GetOptionalUAV(graph, culled.m_stats.Get());
            constants.m_visibilityCapacity = culled.m_visibilityCapacity;
            constants.m_phase = phase;
            constants.m_depthPyramid = CreateDepthPyramidBinding(graph, depthPyramid);
            graph.AddDispatchIndirect("OcclusionCullMeshInstances",
                                      OcclusionCullMeshInstancesPipeline::GetPipeline(),
                                      constants,
                                      GetOcclusionArguments(candidates));
        }


        // Frustum culling and LOD selection run first; a separate pass tests only their survivors against the depth pyramid.
        Core::StructuredBuffer<MeshPass::InstanceClassification> CullInstances(Core::FrameGraph& graph,
                                                                               Core::RingUploader& uploader,
                                                                               const CulledMeshView& culled, const uint32_t phase,
                                                                               const Tools::DepthPyramid::Output* depthPyramid)
        {
            const bool isLate = phase == MeshPass::CullPhase::kLate;
            const Env::Name name(isLate ? "MeshLateClassification" : "MeshClassification");
            auto classification = Core::Buffer::CreateStructured<MeshPass::InstanceClassification>(
                graph.GetDevice(),
                name,
                culled.m_dispatchCount * MeshPass::kInstancesPerCullGroup);

            OcclusionCandidates<uint32_t> candidates;
            if (depthPyramid != nullptr)
            {
                candidates = CreateOcclusionCandidates<uint32_t>(graph,
                                                                 uploader,
                                                                 "MeshInstanceOcclusionCandidates",
                                                                 culled.m_dispatchCount * MeshPass::kInstancesPerCullGroup);
            }

            MeshPass::CullMeshInstances::Constants constants{};
            constants.m_view = culled.m_view;
            constants.m_dispatches = graph.GetSRV(culled.m_dispatches.Get());
            constants.m_classification = graph.GetUAV(classification.Get());
            constants.m_visibility = graph.GetUAV(culled.m_visibility.Get());
            constants.m_occlusionCandidates = GetOptionalUAV(graph, candidates.m_candidates.Get());
            constants.m_occlusionDispatch = GetOptionalUAV(graph, candidates.m_dispatch.Get());
            constants.m_stats = GetOptionalUAV(graph, culled.m_stats.Get());
            constants.m_visibilityCapacity = culled.m_visibilityCapacity;
            constants.m_isOcclusionTested = depthPyramid != nullptr ? 1 : 0;
            constants.m_phase = phase;
            graph.AddDispatch(isLate ? "CullLateMeshInstances" : "CullMeshInstances",
                              CullMeshInstancesPipeline::GetPipeline(),
                              constants,
                              culled.m_dispatchCount);

            if (depthPyramid != nullptr)
                OcclusionCullInstances(graph, culled, phase, *depthPyramid, classification.Get(), candidates);

            return classification;
        }


        void Clear(Core::FrameGraph& graph, const CountOutput& output, const uint32_t bucketCount)
        {
            MeshPass::ClearMeshWork::Constants desc{};
            desc.m_counts = graph.GetUAV(output.m_counts.Get());
            desc.m_cursors = graph.GetUAV(output.m_cursors.Get());
            desc.m_meshletCounts = graph.GetUAV(output.m_meshletCounts.Get());
            desc.m_bucketCount = bucketCount;
            graph.AddDispatch("ClearMeshWork",
                              ClearMeshWorkPipeline::GetPipeline(),
                              desc,
                              Math::CeilDivide(bucketCount * 2, MeshPass::ClearMeshWork::kThreadCount));
        }


        CountOutput Count(Core::FrameGraph& graph, const CountInput& input, const uint32_t bucketCount)
        {
            Core::Device* device = graph.GetDevice();
            CountOutput output;
            output.m_counts = Core::Buffer::CreateStructured<uint32_t>(device, "MeshBucketCounts", bucketCount * 2);
            output.m_cursors = Core::Buffer::CreateStructured<uint32_t>(device, "MeshBucketCursors", bucketCount * 2);
            output.m_meshletCounts = Core::Buffer::CreateStructured<uint32_t>(device, "MeshBucketMeshletCounts", bucketCount);
            output.m_instanceBuckets =
                Core::Buffer::CreateStructured<uint32_t>(device,
                                                         "MeshInstanceBuckets",
                                                         input.m_dispatchCount * MeshPass::kInstancesPerCullGroup);
            Clear(graph, output, bucketCount);

            MeshPass::CountMeshInstances::Constants constants{};
            constants.m_classification = graph.GetSRV(input.m_classification);
            constants.m_routing = graph.GetSRV(input.m_routing);
            constants.m_instanceBuckets = graph.GetUAV(output.m_instanceBuckets.Get());
            constants.m_counts = graph.GetUAV(output.m_counts.Get());
            constants.m_drawTagMask = input.m_drawTagMask;
            graph.AddDispatch("CountEligibleMeshInstances",
                              CountMeshInstancesPipeline::GetPipeline(),
                              constants,
                              input.m_dispatchCount);
            return output;
        }


        Core::StructuredBuffer<uint2> Scan(Core::FrameGraph& graph, const Core::StructuredBufferPtr<uint32_t> counts,
                                           const uint32_t bucketCount)
        {
            struct ScanLevel final
            {
                Core::StructuredBuffer<uint2> m_offsets;
                uint32_t m_count;
            };

            // Scan block sums recursively, then propagate offsets back to the bucket level.
            festd::fixed_vector<ScanLevel, 4> levels;
            // Each bucket counts pixels and waves, which the scan reads as one pair.
            Core::StructuredBuffer<uint2> input = counts.As<uint2>();
            uint32_t count = bucketCount;
            for (;;)
            {
                const uint32_t blocks = Math::CeilDivide(count, MeshPass::ScanMeshBuckets::kThreadCount);
                const auto offsets = Core::Buffer::CreateStructured<Vector2UInt>(graph.GetDevice(), "MeshBucketOffsets", count);
                const auto sums = Core::Buffer::CreateStructured<Vector2UInt>(graph.GetDevice(), "MeshBucketBlockSums", blocks);

                MeshPass::ScanMeshBuckets::Constants constants{};
                constants.m_input = graph.GetSRV(input.Get());
                constants.m_output = graph.GetUAV(offsets.Get());
                constants.m_sums = graph.GetUAV(sums.Get());
                constants.m_count = count;
                graph.AddDispatch("ScanMeshBucketBlocks", ScanMeshBucketsPipeline::GetPipeline(), constants, blocks);

                levels.push_back({ offsets, count });
                if (blocks == 1)
                    break;

                input = sums;
                count = blocks;
            }

            for (uint32_t level = levels.size() - 1; level > 0; --level)
            {
                MeshPass::PropagateMeshBucketOffsets::Constants constants{};
                constants.m_input = graph.GetSRV(levels[level].m_offsets.Get());
                constants.m_output = graph.GetUAV(levels[level - 1].m_offsets.Get());
                constants.m_count = levels[level - 1].m_count;
                graph.AddDispatch("PropagateMeshBucketOffsets",
                                  PropagateMeshBucketOffsetsPipeline::GetPipeline(),
                                  constants,
                                  Math::CeilDivide(constants.m_count, MeshPass::PropagateMeshBucketOffsets::kThreadCount));
            }

            return levels.front().m_offsets;
        }


        FinalizeOutput FinalizeBuckets(Core::FrameGraph& graph, const Core::StructuredBufferPtr<uint32_t> counts,
                                       const Core::StructuredBufferPtr<uint2> offsets, const uint32_t bucketCount)
        {
            Core::Device* device = graph.GetDevice();
            FinalizeOutput output;
            output.m_buckets = Core::Buffer::CreateStructured<MeshPass::PipelineBucket>(device, "MeshBuckets", bucketCount);
            output.m_cullDispatch =
                Core::Buffer::CreateStructured<MeshPass::MeshletCullDispatch>(device, "MeshletCullDispatch", 1);

            MeshPass::FinalizeMeshBuckets::Constants constants{};
            constants.m_offsets = graph.GetSRV(offsets);
            constants.m_counts = graph.GetSRV(counts);
            constants.m_buckets = graph.GetUAV(output.m_buckets.Get());
            constants.m_cullDispatch = graph.GetUAV(output.m_cullDispatch.Get());
            constants.m_bucketCount = bucketCount;
            graph.AddDispatch("FinalizeMeshBuckets",
                              FinalizeMeshBucketsPipeline::GetPipeline(),
                              constants,
                              Math::CeilDivide(bucketCount, MeshPass::FinalizeMeshBuckets::kThreadCount));
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

            MeshPass::ScatterMeshWork::Constants constants{};
            constants.m_classification = graph.GetSRV(input.m_classification);
            constants.m_instanceBuckets = graph.GetSRV(input.m_instanceBuckets);
            constants.m_buckets = graph.GetSRV(input.m_buckets);
            constants.m_cursors = graph.GetUAV(input.m_cursors);
            constants.m_instances = graph.GetUAV(output.m_instances.Get());
            constants.m_work = graph.GetUAV(output.m_work.Get());
            constants.m_stats = GetOptionalUAV(graph, input.m_stats);
            constants.m_phase = input.m_phase;
            graph.AddDispatch("ScatterMeshletWork", ScatterMeshWorkPipeline::GetPipeline(), constants, input.m_dispatchCount);
            return output;
        }


        void OcclusionCullMeshlets(Core::FrameGraph& graph, const MeshletCullInput& input,
                                   const Core::StructuredBufferPtr<MeshPass::VisibleMeshlet> meshlets,
                                   const OcclusionCandidates<MeshPass::MeshletOcclusionCandidate>& candidates)
        {
            MeshPass::OcclusionCullMeshlets::Constants constants{};
            constants.m_view = input.m_view;
            constants.m_instances = graph.GetSRV(input.m_instances);
            constants.m_candidates = graph.GetSRV(candidates.m_candidates.Get());
            constants.m_dispatch = graph.GetSRV(candidates.m_dispatch.Get());
            constants.m_buckets = graph.GetSRV(input.m_buckets);
            constants.m_meshletCounts = graph.GetUAV(input.m_meshletCounts);
            constants.m_meshlets = graph.GetUAV(meshlets);
            constants.m_meshletVisibility = graph.GetUAV(input.m_meshletVisibility);
            constants.m_stats = GetOptionalUAV(graph, input.m_stats);
            constants.m_meshletVisibilityCapacity = input.m_meshletVisibilityCapacity;
            constants.m_phase = input.m_phase;
            constants.m_depthPyramid = CreateDepthPyramidBinding(graph, *input.m_depthPyramid);
            graph.AddDispatchIndirect("OcclusionCullMeshlets",
                                      OcclusionCullMeshletsPipeline::GetPipeline(),
                                      constants,
                                      GetOcclusionArguments(candidates));
        }


        // Frustum culling and the visibility history decide most meshlets; a separate pass tests the frustum survivors that
        // need an occlusion test against the depth pyramid.
        Core::StructuredBuffer<MeshPass::VisibleMeshlet> CullMeshlets(Core::FrameGraph& graph, Core::RingUploader& uploader,
                                                                      const MeshletCullInput& input)
        {
            auto meshlets = Core::Buffer::CreateStructured<MeshPass::VisibleMeshlet>(graph.GetDevice(),
                                                                                     "VisibleMeshlets",
                                                                                     input.m_meshletCapacity);

            OcclusionCandidates<MeshPass::MeshletOcclusionCandidate> candidates;
            if (input.m_depthPyramid != nullptr)
            {
                candidates = CreateOcclusionCandidates<MeshPass::MeshletOcclusionCandidate>(graph,
                                                                                            uploader,
                                                                                            "MeshletOcclusionCandidates",
                                                                                            input.m_meshletCapacity);
            }

            MeshPass::CullMeshlets::Constants constants{};
            constants.m_view = input.m_view;
            constants.m_instances = graph.GetSRV(input.m_instances);
            constants.m_work = graph.GetSRV(input.m_work);
            constants.m_buckets = graph.GetSRV(input.m_buckets);
            constants.m_dispatch = graph.GetSRV(input.m_cullDispatch);
            constants.m_meshletCounts = graph.GetUAV(input.m_meshletCounts);
            constants.m_meshlets = graph.GetUAV(meshlets.Get());
            constants.m_meshletVisibility = graph.GetUAV(input.m_meshletVisibility);
            constants.m_occlusionCandidates = GetOptionalUAV(graph, candidates.m_candidates.Get());
            constants.m_occlusionDispatch = GetOptionalUAV(graph, candidates.m_dispatch.Get());
            constants.m_stats = GetOptionalUAV(graph, input.m_stats);
            constants.m_meshletVisibilityCapacity = input.m_meshletVisibilityCapacity;
            constants.m_isOcclusionTested = input.m_depthPyramid != nullptr ? 1 : 0;
            constants.m_phase = input.m_phase;
            graph.AddDispatchIndirect("CullMeshlets",
                                      CullMeshletsPipeline::GetPipeline(),
                                      constants,
                                      Core::BufferView(input.m_cullDispatch).Slice(0, sizeof(MeshPass::MeshDispatchArguments)));

            if (input.m_depthPyramid != nullptr)
                OcclusionCullMeshlets(graph, input, meshlets.Get(), candidates);

            return meshlets;
        }


        CommandOutput FinalizeCommands(Core::FrameGraph& graph, const CommandInput& input)
        {
            Core::Device* device = graph.GetDevice();
            CommandOutput output;
            output.m_commands =
                Core::Buffer::CreateStructured<MeshPass::MeshCommandRange>(device, "MeshCommandRanges", input.m_commandCapacity);
            output.m_arguments = Core::Buffer::CreateStructured<MeshPass::MeshDispatchArguments>(device,
                                                                                                 "MeshIndirectArguments",
                                                                                                 input.m_commandCapacity);

            MeshPass::FinalizeMeshletCommands::Constants constants{};
            constants.m_buckets = graph.GetSRV(input.m_buckets);
            constants.m_meshletCounts = graph.GetSRV(input.m_meshletCounts);
            constants.m_layouts = graph.GetSRV(input.m_layouts);
            constants.m_commands = graph.GetUAV(output.m_commands.Get());
            constants.m_arguments = graph.GetUAV(output.m_arguments.Get());
            constants.m_bucketCount = input.m_bucketCount;
            graph.AddDispatch("FinalizeMeshletCommands",
                              FinalizeMeshletCommandsPipeline::GetPipeline(),
                              constants,
                              Math::CeilDivide(input.m_bucketCount, MeshPass::FinalizeMeshletCommands::kThreadCount));
            return output;
        }
    } // namespace


    void CullView(Core::FrameGraph& graph, Core::RingUploader& uploader, const uint32_t dispatchCount,
                  const festd::fixed_function<48, void(festd::span<MeshPass::CullDispatch>)>& writeDispatches,
                  const ConstantsPtr<MeshPass::MeshViewData> view, const uint32_t phase, const MeshCullViewResources& resources,
                  CulledMeshView& result)
    {
        FE_PROFILER_ZONE();
        FE_FG_SCOPE(graph, "CullMeshView");
        FE_Assert(phase != MeshPass::CullPhase::kLate);
        if (dispatchCount == 0)
            return;

        Core::Device* device = graph.GetDevice();
        result.m_view = view;
        result.m_dispatchCount = dispatchCount;
        result.m_phase = phase;
        result.m_visibility = resources.m_visibility;
        result.m_visibilityCapacity = resources.m_visibilityCapacity;
        result.m_meshletVisibility = resources.m_meshletVisibility;
        result.m_meshletVisibilityCapacity = resources.m_meshletVisibilityCapacity;
        result.m_stats = resources.m_stats;
        result.m_dispatches = Core::Buffer::CreateStructured<MeshPass::CullDispatch>(device, "MeshCullDispatches", dispatchCount);

        FE_Verify(uploader.UploadGenerated(graph,
                                           result.m_dispatches.Get(),
                                           dispatchCount * sizeof(MeshPass::CullDispatch),
                                           [&](void* target) {
                                               writeDispatches({ static_cast<MeshPass::CullDispatch*>(target), dispatchCount });
                                           }));

        FE_Assert(resources.m_depthPyramid == nullptr || phase == MeshPass::CullPhase::kSingle);
        result.m_classification = CullInstances(graph, uploader, result, phase, resources.m_depthPyramid);
    }


    void CullLateInstances(Core::FrameGraph& graph, Core::RingUploader& uploader, const Tools::DepthPyramid::Output& depthPyramid,
                           CulledMeshView& culled)
    {
        FE_PROFILER_ZONE();
        FE_FG_SCOPE(graph, "CullLateMeshView");
        FE_Assert(culled.m_phase == MeshPass::CullPhase::kEarly);
        if (culled.m_dispatchCount == 0 || culled.m_lateClassification)
            return;

        culled.m_lateClassification = CullInstances(graph, uploader, culled, MeshPass::CullPhase::kLate, &depthPyramid);
    }


    void Build(Core::FrameGraph& graph, Core::RingUploader& uploader, const MeshPipelineRegistry& registry,
               const CulledMeshView& culled, const MeshPassPhase& phase, const Vector2UInt drawTagMask, PreparedMeshPass& result)
    {
        FE_PROFILER_ZONE();
        FE_FG_SCOPE(graph, "PrepareMeshPass");
        if (registry.m_slots.empty() || culled.m_dispatchCount == 0)
            return;

        if (registry.m_workCapacity == 0)
            return;

        const bool isLate = phase.m_phase == MeshPass::CullPhase::kLate;
        const Core::StructuredBufferPtr<MeshPass::InstanceClassification> classification =
            isLate ? culled.m_lateClassification.Get() : culled.m_classification.Get();
        FE_Assert(classification);
        FE_Assert(!isLate || phase.m_depthPyramid != nullptr);

        // Each stage owns its outputs and names only the inputs it consumes.
        const uint32_t bucketCount = registry.m_slots.size();
        const CountInput countInput{ classification, registry.m_routing.Get(), culled.m_dispatchCount, drawTagMask };
        const CountOutput counts = Count(graph, countInput, bucketCount);
        const auto offsets = Scan(graph, counts.m_counts.Get(), bucketCount);
        const FinalizeOutput buckets = FinalizeBuckets(graph, counts.m_counts.Get(), offsets.Get(), bucketCount);

        ScatterInput scatterInput;
        scatterInput.m_classification = classification;
        scatterInput.m_instanceBuckets = counts.m_instanceBuckets.Get();
        scatterInput.m_buckets = buckets.m_buckets.Get();
        scatterInput.m_cursors = counts.m_cursors.Get();
        scatterInput.m_stats = phase.m_stats;
        scatterInput.m_dispatchCount = culled.m_dispatchCount;
        scatterInput.m_instanceCapacity = registry.m_instanceCapacity;
        scatterInput.m_workCapacity = registry.m_workCapacity;
        scatterInput.m_phase = phase.m_phase;
        const ScatterOutput scattered = Scatter(graph, scatterInput);

        MeshletCullInput cullInput;
        cullInput.m_view = culled.m_view;
        cullInput.m_instances = scattered.m_instances.Get();
        cullInput.m_work = scattered.m_work.Get();
        cullInput.m_buckets = buckets.m_buckets.Get();
        cullInput.m_cullDispatch = buckets.m_cullDispatch.Get();
        cullInput.m_meshletCounts = counts.m_meshletCounts.Get();
        cullInput.m_meshletVisibility = culled.m_meshletVisibility.Get();
        cullInput.m_meshletVisibilityCapacity = culled.m_meshletVisibilityCapacity;
        cullInput.m_stats = phase.m_stats;
        cullInput.m_depthPyramid = phase.m_depthPyramid;
        cullInput.m_meshletCapacity = registry.m_workCapacity * MeshPass::kMeshletsPerWorkChunk;
        cullInput.m_phase = phase.m_phase;
        const auto meshlets = CullMeshlets(graph, uploader, cullInput);

        const CommandInput commandInput{ buckets.m_buckets.Get(),
                                         counts.m_meshletCounts.Get(),
                                         registry.m_layouts.Get(),
                                         bucketCount,
                                         registry.m_commandCapacity };
        const CommandOutput commands = FinalizeCommands(graph, commandInput);

        result.m_meshlets = meshlets;
        result.m_commands = commands.m_commands;
        result.m_arguments = commands.m_arguments;
        result.m_submissions = registry.m_submissions;
        result.m_view = culled.m_view;

        MeshPass::MeshPassData passData{};
        passData.m_visibleMeshlets = graph.GetSRV(result.m_meshlets.Get());
        passData.m_commands = graph.GetSRV(result.m_commands.Get());
        passData.m_passData = phase.m_passData;
        result.m_pass = graph.CreateConstants(passData);
    }
} // namespace FE::Graphics::GpuMeshWorkBuilder
