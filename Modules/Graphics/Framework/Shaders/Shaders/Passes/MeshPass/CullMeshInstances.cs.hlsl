#include <Shaders/Core/Culling/Culling.hlsli>
#include <Shaders/Passes/MeshPass/CullMeshInstances.h>

[[vk::push_constant]] Constants GConstants;

void CountBucket(uint bucket, uint workCount)
{
    if (bucket == kInvalidIndex)
        return;

    RWStructuredBuffer<uint> counts = GConstants.m_counts.Get();
    InterlockedAdd(counts[bucket * 2], 1);
    InterlockedAdd(counts[bucket * 2 + 1], workCount);
}


void ClassifyMember(ViewData view, DB::Slice<MeshMemberTable> members, uint passMask, uint scratchIndex, uint memberIndex)
{
    InstanceClassification item = (InstanceClassification)0;
    item.m_depthBucket = kInvalidIndex;
    item.m_opaqueBucket = kInvalidIndex;
    if (memberIndex >= members.m_count)
    {
        GConstants.m_classification.Store(scratchIndex, item);
        return;
    }

    MeshMemberTable membership = MeshMemberTable::Create(view.m_members);
    MeshInstanceTable instances = MeshInstanceTable::Create(view.m_instances);
    MeshGroupTable groups = MeshGroupTable::Create(view.m_groups);
    MeshLodInfoTable lods = MeshLodInfoTable::Create(view.m_lods);
    const DB::Ref<MeshInstanceTable> instanceId = membership.ReadRow(members.m_rowIndex + memberIndex).m_instance.Get();
    const MeshInstanceTable::Row instance = instances.ReadRow(instanceId);
    const DB::Ref<MeshGroupTable> groupId = instance.m_meshGroup.Get();
    const MeshGroupTable::Row mesh = groups.ReadRow(groupId);
    if (mesh.m_renderable.Get() != 0)
    {
        const uint lodId = mesh.m_lods.Get().m_rowIndex;
        const uint meshletCount = lods.ReadRow(lodId).m_info.Get().m_meshletCount;
        const float4x4 localToClip = mul(instance.m_transform.Get(), view.m_viewProjection);
        const bool visible = VisibleBounds(mesh.m_bounds.Get().m_min, mesh.m_bounds.Get().m_max, localToClip);
        if (meshletCount != 0 && visible)
        {
            const PipelineRouting routing = view.m_routing.Load(groupId.m_rowIndex);

            item.m_instanceId = instanceId.m_rowIndex;
            item.m_lodId = lodId;
            item.m_meshletCount = meshletCount;
            item.m_depthBucket = (passMask & 1) != 0 ? routing.m_depthBucket : kInvalidIndex;
            item.m_opaqueBucket = (passMask & 2) != 0 ? routing.m_opaqueBucket : kInvalidIndex;
            const uint workCount = (meshletCount + kMeshletsPerWorkChunk - 1) / kMeshletsPerWorkChunk;
            CountBucket(item.m_depthBucket, workCount);
            CountBucket(item.m_opaqueBucket, workCount);
        }
    }
    GConstants.m_classification.Store(scratchIndex, item);
}


FE_NUM_THREADS(kInstancesPerCullGroup, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const ViewData view = GConstants.m_view.Load(0);
    const CullDispatch dispatch = GConstants.m_dispatches.Load(group.x);
    MeshBatchTable batches = MeshBatchTable::Create(view.m_batches);
    const MeshBatchTable::Row batch = batches.ReadRow(dispatch.m_batchId);

    const DB::Slice<MeshMemberTable> members = batch.m_members.Get();
    const uint passMask = batch.m_passMask.Get() & view.m_enabledPasses;
    const uint scratchBase = group.x * kInstancesPerCullGroup;
    ClassifyMember(view, members, passMask, scratchBase + lane, dispatch.m_firstMember + lane);
}
