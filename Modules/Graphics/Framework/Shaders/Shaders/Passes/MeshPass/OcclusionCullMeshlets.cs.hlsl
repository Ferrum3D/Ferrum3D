#include <Shaders/Passes/MeshPass/CullCommon.hlsli>
#include <Shaders/Passes/MeshPass/CullStats.hlsli>
#include <Shaders/Passes/MeshPass/OcclusionCullMeshlets.h>

[[vk::push_constant]] Constants GConstants;

bool IsMeshletOccluded(const MeshViewData view, const float4 worldSphere)
{
    ScreenBounds screenBounds;
    if (!ProjectBox(worldSphere.xyz, worldSphere.w, view.m_cull.m_occlusionViewProjection, screenBounds))
        return false;

    const DepthPyramid pyramid = CreateDepthPyramid(GConstants.m_depthPyramid,
                                                    GConstants.m_depthPyramidSize,
                                                    GConstants.m_depthPyramidUvScale,
                                                    GConstants.m_depthPyramidMipCount);
    return IsOccluded(pyramid, screenBounds);
}


FE_NUM_THREADS(kOcclusionCullThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint candidateIndex = GetOcclusionCandidateIndex(lane, group);
    const bool isCandidate = candidateIndex < GConstants.m_dispatch.Load(0).m_candidateCount;

    VisibleInstance instance = (VisibleInstance)0;
    uint meshletIndex = 0;
    bool isVisible = false;
    bool isOccluded = false;
    uint triangleCount = 0;
    if (isCandidate)
    {
        const MeshViewData view = LoadMeshView(GConstants.m_view);
        const MeshletOcclusionCandidate candidate = GConstants.m_candidates.Load(candidateIndex);
        instance = GConstants.m_instances.Load(candidate.m_visibleInstanceIndex);
        meshletIndex = candidate.m_meshlet & ~kCandidateRenderedEarlyBit;

        const MeshDrawData draw = ResolveMeshDrawData(view, instance.m_instanceId, instance.m_lodIndex);
        isOccluded = IsMeshletOccluded(view, LoadMeshletWorldSphere(draw, meshletIndex));

        const uint wordIndex = GetMeshletHistoryWordIndex(draw, meshletIndex, GConstants.m_meshletVisibilityCapacity);
        if (GConstants.m_phase == CullPhase::kLate && !isOccluded && wordIndex != kInvalidIndex)
            InterlockedOr(GConstants.m_meshletVisibility.Get()[wordIndex], 1u << (meshletIndex % 32));

        // Meshlets rendered early are only tested to record their visibility; a meshlet no phase renders is occluded.
        const bool wasRenderedEarly = (candidate.m_meshlet & kCandidateRenderedEarlyBit) != 0;
        isVisible = !isOccluded && !wasRenderedEarly;
        isOccluded = isOccluded && !wasRenderedEarly;
        if (isVisible && GConstants.m_stats.IsValid())
            triangleCount = LoadMeshletTriangleCount(draw, meshletIndex);
    }

    AppendVisibleMeshlet(GConstants.m_meshletCounts,
                         GConstants.m_meshlets,
                         GConstants.m_buckets,
                         instance,
                         meshletIndex,
                         isVisible);

    if (!GConstants.m_stats.IsValid())
        return;

    AddCullStat(GConstants.m_stats, CullStat::kMeshletOcclusionCulled, isOccluded);
    AddCullStat(GConstants.m_stats, CullStat::kMeshletRendered, isVisible);
    AddCullStatValue(GConstants.m_stats, CullStat::kTriangleRendered, triangleCount);
}
