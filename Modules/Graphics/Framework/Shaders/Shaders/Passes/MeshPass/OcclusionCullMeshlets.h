#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::OcclusionCullMeshlets)
    //! Tests the frustum-visible meshlets appended by CullMeshlets against the depth pyramid. Visible meshlets the early phase
    //! did not render are appended to the meshlet range of their bucket; in the late phase, visible meshlets also set their
    //! visibility bit. m_stats is invalid for passes that do not report statistics.
    struct Constants final
    {
        ConstantsPtr<MeshViewData> m_view;
        StructuredBufferDescriptor<VisibleInstance> m_instances;
        StructuredBufferDescriptor<MeshletOcclusionCandidate> m_candidates;
        StructuredBufferDescriptor<OcclusionCullDispatch> m_dispatch;
        StructuredBufferDescriptor<PipelineBucket> m_buckets;
        RWStructuredBufferDescriptor<uint32_t> m_meshletCounts;
        RWStructuredBufferDescriptor<VisibleMeshlet> m_meshlets;
        RWStructuredBufferDescriptor<uint32_t> m_meshletVisibility;
        RWStructuredBufferDescriptor<uint32_t> m_stats;
        DepthPyramidBinding m_depthPyramid;
        //! One pyramid per view of a multi-view pass, which replaces m_depthPyramid; invalid otherwise.
        ConstantsPtr<DepthPyramidBinding> m_viewPyramids;
        uint32_t m_meshletVisibilityCapacity;
        uint32_t m_phase;

        FE_RTTI_Reflect("506790E0-C086-47D8-9B18-DB522C75C2E5");
    };
FE_HOST_END_NAMESPACE
