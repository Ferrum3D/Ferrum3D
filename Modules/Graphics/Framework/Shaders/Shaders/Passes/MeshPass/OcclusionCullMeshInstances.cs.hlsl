#include <Shaders/Passes/MeshPass/CullCommon.hlsli>
#include <Shaders/Passes/MeshPass/CullStats.hlsli>
#include <Shaders/Passes/MeshPass/OcclusionCullMeshInstances.h>

[[vk::push_constant]] Constants GConstants;

bool IsInstanceOccluded(const MeshViewData view, const InstanceClassification item)
{
    const MeshSceneData scene = LoadMeshScene(view.m_scene);
    const MeshInstanceTable::Row instance = MeshInstanceTable::Create(scene.m_instances).ReadRow(item.m_instanceId);
    const MeshGroupTable::Row group = MeshGroupTable::Create(scene.m_groups).ReadRow(item.m_groupId);
    const InstanceBounds bounds = LoadInstanceBounds(group);

    ScreenBounds screenBounds;
    const float4x4 localToClip = mul(instance.m_transform.Get(), view.m_cull.m_occlusionViewProjection);
    if (!ProjectBox(bounds.m_center, bounds.m_extent, localToClip, screenBounds))
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

    bool isOccluded = false;
    if (isCandidate)
    {
        const MeshViewData view = LoadMeshView(GConstants.m_view);
        const uint slot = GConstants.m_candidates.Load(candidateIndex);
        const InstanceClassification item = GConstants.m_classification.Load(slot);
        isOccluded = IsInstanceOccluded(view, item);
        if (isOccluded)
        {
            GConstants.m_classification.Store(slot, (InstanceClassification)0);
            if (GConstants.m_phase == CullPhase::kLate && item.m_instanceId < GConstants.m_visibilityCapacity)
                GConstants.m_visibility.Store(item.m_instanceId, 0);
        }
    }

    AddCullStat(GConstants.m_stats, CullStat::kInstanceOcclusionCulled, isOccluded);
}
