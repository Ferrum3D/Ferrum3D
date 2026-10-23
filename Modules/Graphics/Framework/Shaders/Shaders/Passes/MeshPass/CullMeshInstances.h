#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::CullMeshInstances)
    //! Frustum culling and LOD selection. m_visibility stores the InstanceVisibility word of each instance row.
    //!
    //! When m_isOcclusionTested is set (the late phase, or the single phase with a frozen depth pyramid), frustum-visible
    //! instances append their classification slot to m_occlusionCandidates for OcclusionCullMeshInstances.
    struct Constants final
    {
        StructuredBufferDescriptor<MeshViewData> m_view;
        StructuredBufferDescriptor<CullDispatch> m_dispatches;
        RWStructuredBufferDescriptor<InstanceClassification> m_classification;
        RWStructuredBufferDescriptor<uint32_t> m_visibility;
        RWStructuredBufferDescriptor<uint32_t> m_occlusionCandidates;
        RWStructuredBufferDescriptor<OcclusionCullDispatch> m_occlusionDispatch;
        RWStructuredBufferDescriptor<uint32_t> m_stats;
        uint32_t m_visibilityCapacity;
        uint32_t m_isOcclusionTested;
        uint32_t m_phase;

        FE_RTTI_Reflect("818EF35C-7444-45F2-B6A5-8C97A016F41C");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;

        FE_RTTI_Reflect("C4F922E3-773F-41CE-867A-C4B0B34DC424");
    };
FE_HOST_END_NAMESPACE
