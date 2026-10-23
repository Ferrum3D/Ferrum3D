#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::CullMeshlets)
    //! One group frustum culls the meshlets of one work chunk and appends the survivors to the meshlet range of its bucket.
    //!
    //! m_meshletVisibility stores one bit per meshlet of each instance's selected LOD, starting at the instance's word offset.
    //! The early phase renders the meshlets visible in the previous frame. When m_isOcclusionTested is set (the late phase, or
    //! the single phase with a frozen depth pyramid) and meshlet occlusion culling is enabled, frustum-visible meshlets are
    //! appended to m_occlusionCandidates instead, and OcclusionCullMeshlets renders and records the visible ones. m_stats is
    //! invalid for passes that do not report statistics.
    struct Constants final
    {
        StructuredBufferDescriptor<MeshViewData> m_view;
        StructuredBufferDescriptor<VisibleInstance> m_instances;
        StructuredBufferDescriptor<MeshletWorkChunk> m_work;
        StructuredBufferDescriptor<PipelineBucket> m_buckets;
        StructuredBufferDescriptor<MeshletCullDispatch> m_dispatch;
        RWStructuredBufferDescriptor<uint32_t> m_meshletCounts;
        RWStructuredBufferDescriptor<VisibleMeshlet> m_meshlets;
        RWStructuredBufferDescriptor<uint32_t> m_meshletVisibility;
        RWStructuredBufferDescriptor<MeshletOcclusionCandidate> m_occlusionCandidates;
        RWStructuredBufferDescriptor<OcclusionCullDispatch> m_occlusionDispatch;
        RWStructuredBufferDescriptor<uint32_t> m_stats;
        uint32_t m_meshletVisibilityCapacity;
        uint32_t m_isOcclusionTested;
        uint32_t m_phase;

        FE_RTTI_Reflect("2C78344A-4394-46E5-BDFE-69A32663EF5C");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;
        Core::PassIndirectArgs m_arguments;

        FE_RTTI_Reflect("302094E9-72EC-40E4-8382-EF16D3E5BAB9");
    };
FE_HOST_END_NAMESPACE
