#include <Shaders/Passes/MeshPass/CullCommon.hlsli>
#include <Shaders/Passes/MeshPass/CullMeshInstances.h>
#include <Shaders/Passes/MeshPass/CullStats.hlsli>

[[vk::push_constant]] Constants GConstants;

struct InstanceCullResult
{
    InstanceClassification m_item;
    bool m_isCandidate;
    bool m_isFrustumCulled;
};


// Choose the coarsest resident LOD whose projected error stays below the threshold. The distance to the bounding sphere
// surface underestimates the distance to the mesh, so the projected error is never underestimated.
uint SelectLod(const MeshCullData cull, MeshLodInfoTable lods, const MeshGroupTable::Row group, const float4x4 localToWorld,
               const InstanceBounds bounds)
{
    const uint finestLod = group.m_finestResidentLod.Get();
    if (!HasCullFlag(cull, CullFlags::kLodSelection))
        return finestLod;

    const float4 sphere = TransformSphere(float4(bounds.m_center, length(bounds.m_extent)), localToWorld);
    const float distance = length(sphere.xyz - cull.m_cameraPosition.xyz) - sphere.w;
    if (distance <= 0.0f)
        return finestLod;

    const DB::Slice<MeshLodInfoTable> slice = group.m_lods.Get();
    const float errorScale = MaxAxisScale(localToWorld) * cull.m_cameraPosition.w;
    uint selectedLod = finestLod;
    for (uint lod = finestLod + 1; lod < slice.m_count; ++lod)
    {
        if (lods.ReadRow(slice.m_rowIndex + lod).m_error.Get() * errorScale > distance)
            break;

        selectedLod = lod;
    }

    return selectedLod;
}


uint LoadVisibility(const uint instanceId)
{
    return instanceId < GConstants.m_visibilityCapacity ? GConstants.m_visibility.Load(instanceId) : 0;
}


void StoreVisibility(const uint instanceId, const uint visibility)
{
    if (instanceId < GConstants.m_visibilityCapacity)
        GConstants.m_visibility.Store(instanceId, visibility);
}


// The early phase renders instances that passed occlusion culling last frame. The late phase classifies every frustum-visible
// instance, including those rendered early, so their meshlets can be tested and recorded as well. Its visibility word is
// stored as visible here; the occlusion pass clears it again for occluded instances.
InstanceCullResult CullMember(const MeshViewData view, const MeshSceneData scene, const DB::Slice<MeshMemberTable> members,
                              const uint memberIndex, const uint2 drawTagMask)
{
    InstanceCullResult result = (InstanceCullResult)0;
    if (memberIndex >= members.m_count)
        return result;

    MeshMemberTable membership = MeshMemberTable::Create(scene.m_members);
    MeshInstanceTable instances = MeshInstanceTable::Create(scene.m_instances);
    MeshGroupTable groups = MeshGroupTable::Create(scene.m_groups);
    MeshLodInfoTable lods = MeshLodInfoTable::Create(scene.m_lods);

    const DB::Ref<MeshInstanceTable> instanceId = membership.ReadRow(members.m_rowIndex + memberIndex).m_instance.Get();
    const MeshInstanceTable::Row instance = instances.ReadRow(instanceId);
    const DB::Ref<MeshGroupTable> groupId = instance.m_meshGroup.Get();
    const MeshGroupTable::Row group = groups.ReadRow(groupId);
    if (group.m_renderable.Get() == 0)
        return result;

    result.m_isCandidate = true;

    const float4x4 localToWorld = instance.m_transform.Get();
    const InstanceBounds bounds = LoadInstanceBounds(group);
    const MeshCullData cull = view.m_cull;
    if (HasCullFlag(cull, CullFlags::kInstanceFrustum))
    {
        const float3 boundsMin = bounds.m_center - bounds.m_extent;
        const float3 boundsMax = bounds.m_center + bounds.m_extent;
        if (!VisibleBounds(boundsMin, boundsMax, mul(localToWorld, cull.m_viewProjection)))
        {
            result.m_isFrustumCulled = true;
            if (GConstants.m_phase == CullPhase::kLate)
                StoreVisibility(instanceId.m_rowIndex, 0);

            return result;
        }
    }

    const uint visibility = LoadVisibility(instanceId.m_rowIndex);
    const bool wasVisible = (visibility & InstanceVisibility::kVisible) != 0;
    if (GConstants.m_phase == CullPhase::kEarly && !wasVisible)
        return result;

    const uint lodIndex = SelectLod(cull, lods, group, localToWorld, bounds);

    // Meshlet visibility bits are only meaningful for the LOD they were recorded with.
    const bool hasMeshletHistory = instance.m_meshletVisibilityOffset.Get() != kInvalidIndex
        && (visibility >> InstanceVisibility::kLodShift) == lodIndex + 1;
    if (GConstants.m_phase == CullPhase::kLate)
        StoreVisibility(instanceId.m_rowIndex, InstanceVisibility::kVisible | ((lodIndex + 1) << InstanceVisibility::kLodShift));

    const uint lodId = group.m_lods.Get().m_rowIndex + lodIndex;
    const uint meshletCount = lods.ReadRow(lodId).m_info.Get().m_meshletCount;
    if (meshletCount == 0)
        return result;

    uint flags = hasMeshletHistory ? InstanceFlags::kMeshletHistory : 0;
    if (GConstants.m_phase == CullPhase::kEarly || (GConstants.m_phase == CullPhase::kLate && wasVisible))
        flags |= InstanceFlags::kRenderedEarly;

    result.m_item.m_instanceId = instanceId.m_rowIndex;
    result.m_item.m_lodId = lodId;
    result.m_item.m_lodIndex = lodIndex;
    result.m_item.m_groupId = groupId.m_rowIndex;
    result.m_item.m_drawTagMask = drawTagMask;
    result.m_item.m_meshletCount = meshletCount;
    result.m_item.m_flags = flags;
    return result;
}


FE_NUM_THREADS(kInstancesPerCullGroup, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const MeshViewData view = LoadMeshView(GConstants.m_view);
    const MeshSceneData scene = LoadMeshScene(view.m_scene);
    const CullDispatch dispatch = GConstants.m_dispatches.Load(group.x);
    MeshBatchTable batches = MeshBatchTable::Create(scene.m_batches);
    const MeshBatchTable::Row batch = batches.ReadRow(dispatch.m_batchId);

    const DB::Slice<MeshMemberTable> members = batch.m_members.Get();
    const uint2 drawTagMask = batch.m_drawTagMask.Get();
    const uint memberIndex = dispatch.m_firstMember + lane;
    const uint slot = group.x * kInstancesPerCullGroup + lane;
    const InstanceCullResult result = CullMember(view, scene, members, memberIndex, drawTagMask);
    GConstants.m_classification.Store(slot, result.m_item);

    if (GConstants.m_isOcclusionTested != 0 && HasCullFlag(view.m_cull, CullFlags::kInstanceOcclusion))
    {
        const bool isAppended = result.m_item.m_meshletCount != 0;
        const uint candidateIndex = AppendOcclusionCandidate(GConstants.m_occlusionDispatch, isAppended);
        if (isAppended)
            GConstants.m_occlusionCandidates.Store(candidateIndex, slot);
    }

    // Both phases see the same candidates and frustum results; count them once.
    const bool isFirstPhase = GConstants.m_phase != CullPhase::kLate;
    AddCullStat(GConstants.m_stats, CullStat::kInstanceCandidates, isFirstPhase && result.m_isCandidate);
    AddCullStat(GConstants.m_stats, CullStat::kInstanceFrustumCulled, isFirstPhase && result.m_isFrustumCulled);
}
