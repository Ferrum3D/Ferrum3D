#include <Shaders/Passes/MeshPass/ScatterMeshWork.h>

[[vk::push_constant]] Constants GConstants;

void ScatterBucket(InstanceClassification item, uint bucket)
{
    if (bucket == kInvalidIndex)
        return;

    const PipelineBucket range = GConstants.m_buckets.Load(bucket);
    const uint workCount = (item.m_meshletCount + kMeshletsPerWorkChunk - 1) / kMeshletsPerWorkChunk;

    RWStructuredBuffer<uint> cursors = GConstants.m_cursors.Get();
    uint instanceOffset, workOffset;
    InterlockedAdd(cursors[bucket * 2], 1, instanceOffset);
    InterlockedAdd(cursors[bucket * 2 + 1], workCount, workOffset);

    VisibleInstance instance;
    instance.m_instanceId = item.m_instanceId;
    instance.m_lodId = item.m_lodId;

    const uint instanceIndex = range.m_firstInstance + instanceOffset;
    GConstants.m_instances.Store(instanceIndex, instance);
    for (uint chunk = 0; chunk < workCount; ++chunk)
    {
        MeshletWorkChunk work;
        work.m_visibleInstanceIndex = instanceIndex;
        work.m_firstMeshlet = chunk * kMeshletsPerWorkChunk;
        GConstants.m_work.Store(range.m_firstWorkChunk + workOffset + chunk, work);
    }
}

FE_NUM_THREADS(kInstancesPerCullGroup, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const InstanceClassification item = GConstants.m_classification.Load(group.x * kInstancesPerCullGroup + lane);
    ScatterBucket(item, item.m_depthBucket);
    ScatterBucket(item, item.m_opaqueBucket);
}
