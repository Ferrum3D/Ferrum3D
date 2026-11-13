#pragma once
#include <Shaders/Passes/MeshPass/GpuWork.h>

FE_HOST_BEGIN_NAMESPACE(FE::Graphics::MultiViewProbe)
#if FE_HOST
    using MeshPass::DepthPyramidBinding;
    using MeshPass::MeshViewData;
#endif

    FE_CONSTEXPR uint32_t kWordsPerView = 4;

    //! Writes kWordsPerView words per view of a multi-view pass, loaded the way its culling passes load them: the viewport
    //! width through LoadMeshView, the cull flags and LOD scale through LoadCandidateMeshView, and the mip count of the
    //! pyramid of the view. Each view is addressed through instance flags that also carry unrelated bits.
    struct Constants final
    {
        ConstantsPtr<MeshViewData> m_views;
        ConstantsPtr<DepthPyramidBinding> m_pyramids;
        RWStructuredBufferDescriptor<uint32_t> m_output;
        uint32_t m_viewCount;

        FE_RTTI_Reflect("A5720412-85BF-4E05-8B75-9671A34225CE");
    };

FE_HOST_END_NAMESPACE
