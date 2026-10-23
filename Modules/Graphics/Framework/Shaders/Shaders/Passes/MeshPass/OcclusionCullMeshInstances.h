#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MeshPass::OcclusionCullMeshInstances)
    //! Tests the frustum-visible instances appended by CullMeshInstances against the depth pyramid. Occluded instances have
    //! their classification slot cleared and, in the late phase, their visibility word reset.
    struct Constants final
    {
        StructuredBufferDescriptor<MeshViewData> m_view;
        StructuredBufferDescriptor<uint32_t> m_candidates;
        StructuredBufferDescriptor<OcclusionCullDispatch> m_dispatch;
        RWStructuredBufferDescriptor<InstanceClassification> m_classification;
        RWStructuredBufferDescriptor<uint32_t> m_visibility;
        RWStructuredBufferDescriptor<uint32_t> m_stats;
        Texture2DDescriptor<float> m_depthPyramid;
        uint32_t m_visibilityCapacity;
        uint2 m_depthPyramidSize;
        float2 m_depthPyramidUvScale;
        uint32_t m_depthPyramidMipCount;
        uint32_t m_phase;

        FE_RTTI_Reflect("F176BF50-D62D-46ED-9211-8EE6B1C45959");
    };


    struct PassDesc final
    {
        Constants m_constants;
        Core::PassComputePipeline m_pipeline;
        Core::PassIndirectArgs m_arguments;

        FE_RTTI_Reflect("74F4A270-390F-4A91-B5FE-AEAEEEE4090C");
    };
FE_HOST_END_NAMESPACE
