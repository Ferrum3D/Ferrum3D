#include <Shaders/Passes/MeshPass/CountMeshInstances.h>

[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(kInstancesPerCullGroup, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint index = group.x * kInstancesPerCullGroup + lane;
    const InstanceClassification item = GConstants.m_classification.Load(index);
    uint bucket = kInvalidIndex;
    if (item.m_meshletCount != 0 && any(item.m_drawTagMask & GConstants.m_drawTagMask))
        bucket = GConstants.m_routing.Load(item.m_groupId).m_bucket;

    GConstants.m_instanceBuckets.Store(index, bucket);
    if (bucket == kInvalidIndex)
        return;

    const uint workCount = (item.m_meshletCount + kMeshletsPerWorkChunk - 1) / kMeshletsPerWorkChunk;
    RWStructuredBuffer<uint> counts = GConstants.m_counts.Get();
    InterlockedAdd(counts[bucket * 2], 1);
    InterlockedAdd(counts[bucket * 2 + 1], workCount);
}
