#include <Shaders/Passes/MeshPass/CullCommon.hlsli>
#include <Shaders/Tests/MultiViewProbe.h>

[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(1, 1, 1)
void main()
{
    for (uint viewIndex = 0; viewIndex < GConstants.m_viewCount; ++viewIndex)
    {
        const uint flags = (viewIndex << InstanceFlags::kViewShift) | InstanceFlags::kRenderedEarly | InstanceFlags::kMeshletHistory;
        const MeshViewData uniformView = LoadMeshView(GConstants.m_views, GetViewIndex(flags));
        const MeshViewData candidateView = LoadCandidateMeshView(GConstants.m_views, GConstants.m_pyramids, flags);
        const DepthPyramidBinding pyramid = GConstants.m_pyramids.LoadElement(GetViewIndex(flags));

        const uint firstWord = viewIndex * kWordsPerView;
        GConstants.m_output.Store(firstWord, (uint)uniformView.m_view.m_viewport.z);
        GConstants.m_output.Store(firstWord + 1, candidateView.m_cull.m_flags);
        GConstants.m_output.Store(firstWord + 2, (uint)candidateView.m_cull.m_cameraPosition.w);
        GConstants.m_output.Store(firstWord + 3, pyramid.m_mipCount);
    }
}
