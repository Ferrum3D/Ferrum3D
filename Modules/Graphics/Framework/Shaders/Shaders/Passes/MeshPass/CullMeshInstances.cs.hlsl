#include <Shaders/Core/Culling/Culling.hlsli>
#include <Shaders/Passes/MeshPass/CullMeshInstances.h>

[[vk::push_constant]] Constants GConstants;

void ClassifyMember(ViewData view, DB::Slice<MeshMemberTable> members, uint2 drawTagMask, uint scratchIndex, uint memberIndex)
{
    InstanceClassification item = (InstanceClassification)0;
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
            item.m_instanceId = instanceId.m_rowIndex;
            item.m_lodId = lodId;
            item.m_meshletCount = meshletCount;
            item.m_groupId = groupId.m_rowIndex;
            item.m_drawTagMask = drawTagMask;
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
    const uint2 drawTagMask = batch.m_drawTagMask.Get();
    const uint scratchBase = group.x * kInstancesPerCullGroup;
    ClassifyMember(view, members, drawTagMask, scratchBase + lane, dispatch.m_firstMember + lane);
}
