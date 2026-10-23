#include <Shaders/Passes/MeshPass/CullCommon.hlsli>
#include <Shaders/Passes/MeshPass/CullMeshlets.h>
#include <Shaders/Passes/MeshPass/CullStats.hlsli>

[[vk::push_constant]] Constants GConstants;

struct MeshletCullResult
{
    bool m_isCandidate;
    bool m_isFrustumCulled;
};


MeshletCullResult CullMeshlet(const MeshViewData view, const MeshDrawData draw, const uint meshletIndex)
{
    MeshletCullResult result = (MeshletCullResult)0;
    if (meshletIndex >= draw.m_lodInfo.m_meshletCount)
        return result;

    result.m_isCandidate = true;
    if (HasCullFlag(view.m_cull, CullFlags::kMeshletFrustum))
        result.m_isFrustumCulled = !VisibleSphere(LoadMeshletWorldSphere(draw, meshletIndex), view.m_cull.m_viewProjection);

    return result;
}


FE_NUM_THREADS(kMeshletsPerWorkChunk, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint chunkIndex = group.y * kMaxGroupsPerDimension + group.x;
    if (chunkIndex >= GConstants.m_dispatch.Load(0).m_workChunkCount)
        return;

    const MeshViewData view = LoadMeshView(GConstants.m_view);
    const MeshletWorkChunk work = GConstants.m_work.Load(chunkIndex);
    const VisibleInstance instance = GConstants.m_instances.Load(work.m_visibleInstanceIndex);
    const MeshDrawData draw = ResolveMeshDrawData(view, instance.m_instanceId, instance.m_lodIndex);
    const uint meshletIndex = work.m_firstMeshlet + lane;
    const MeshletCullResult result = CullMeshlet(view, draw, meshletIndex);
    const bool isFrustumVisible = result.m_isCandidate && !result.m_isFrustumCulled;

    // Work chunks start at multiples of 32 meshlets, so each chunk owns one word of its instance's visibility bits.
    const uint historyWordIndex = GetMeshletHistoryWordIndex(draw, work.m_firstMeshlet, GConstants.m_meshletVisibilityCapacity);
    const bool hasHistory = (instance.m_flags & InstanceFlags::kMeshletHistory) != 0 && historyWordIndex != kInvalidIndex;
    const uint previousBits = hasHistory ? GConstants.m_meshletVisibility.Load(historyWordIndex) : 0;
    const bool wasVisible = ((previousBits >> lane) & 1) != 0;

    // The late phase recomputes which meshlets the early phase rendered from the same inputs, so none is rendered twice
    // and none is skipped by both phases.
    const bool isRenderedEarly = (instance.m_flags & InstanceFlags::kRenderedEarly) != 0;
    const bool wasRenderedEarly = isRenderedEarly && wasVisible && isFrustumVisible;

    const bool isOcclusionTested = GConstants.m_isOcclusionTested != 0 && HasCullFlag(view.m_cull, CullFlags::kMeshletOcclusion);
    const bool needsOcclusionTest = isOcclusionTested && isFrustumVisible;
    bool isVisible = isFrustumVisible && !needsOcclusionTest;
    if (GConstants.m_phase == CullPhase::kEarly)
        isVisible = wasRenderedEarly;
    else if (GConstants.m_phase == CullPhase::kLate)
        isVisible = isVisible && !wasRenderedEarly;

    // The late phase records the meshlets visible without an occlusion test; the occlusion pass adds the others.
    if (GConstants.m_phase == CullPhase::kLate)
    {
        const uint visibleBits = WaveActiveBallot(isFrustumVisible && !needsOcclusionTest).x;
        if (WaveIsFirstLane() && historyWordIndex != kInvalidIndex)
            GConstants.m_meshletVisibility.Store(historyWordIndex, visibleBits);
    }

    const uint candidateIndex = AppendOcclusionCandidate(GConstants.m_occlusionDispatch, needsOcclusionTest);
    if (needsOcclusionTest)
    {
        MeshletOcclusionCandidate candidate;
        candidate.m_visibleInstanceIndex = work.m_visibleInstanceIndex;
        candidate.m_meshlet = meshletIndex | (wasRenderedEarly ? kCandidateRenderedEarlyBit : 0);
        GConstants.m_occlusionCandidates.Store(candidateIndex, candidate);
    }

    AppendVisibleMeshlet(GConstants.m_meshletCounts,
                         GConstants.m_meshlets,
                         GConstants.m_buckets,
                         instance,
                         meshletIndex,
                         isVisible);

    if (!GConstants.m_stats.IsValid())
        return;

    // Meshlets of instances revisited by the late phase were counted by the early phase.
    const bool isCounted = GConstants.m_phase != CullPhase::kLate || !isRenderedEarly;
    const uint triangleCount = isVisible ? LoadMeshletTriangleCount(draw, meshletIndex) : 0;
    AddCullStat(GConstants.m_stats, CullStat::kMeshletCandidates, isCounted && result.m_isCandidate);
    AddCullStat(GConstants.m_stats, CullStat::kMeshletFrustumCulled, isCounted && result.m_isFrustumCulled);
    AddCullStat(GConstants.m_stats, CullStat::kMeshletRendered, isVisible);
    AddCullStatValue(GConstants.m_stats, CullStat::kTriangleRendered, triangleCount);
}
