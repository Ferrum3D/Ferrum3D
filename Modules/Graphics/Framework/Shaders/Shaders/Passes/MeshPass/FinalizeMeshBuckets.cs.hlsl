#include <Shaders/Passes/MeshPass/FinalizeMeshBuckets.h>

[[vk::push_constant]] Constants GConstants;

// Buckets are laid out in order, so the last bucket's range ends at the total work chunk count.
void WriteCullDispatch(const PipelineBucket lastBucket)
{
    const uint workChunkCount = lastBucket.m_firstWorkChunk + lastBucket.m_workChunkCount;

    MeshletCullDispatch dispatch;
    dispatch.m_groupCountX = min(workChunkCount, kMaxGroupsPerDimension);
    dispatch.m_groupCountY = (workChunkCount + kMaxGroupsPerDimension - 1) / kMaxGroupsPerDimension;
    dispatch.m_groupCountZ = 1;
    dispatch.m_workChunkCount = workChunkCount;
    GConstants.m_cullDispatch.Store(0, dispatch);
}


FE_NUM_THREADS(kThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint bucket = group.x * kThreadCount + lane;
    if (bucket >= GConstants.m_bucketCount)
        return;

    const uint2 offset = GConstants.m_offsets.Load(bucket);
    PipelineBucket range;
    range.m_firstInstance = offset.x;
    range.m_firstWorkChunk = offset.y;
    range.m_instanceCount = GConstants.m_counts.Load(bucket * 2);
    range.m_workChunkCount = GConstants.m_counts.Load(bucket * 2 + 1);
    GConstants.m_buckets.Store(bucket, range);

    if (bucket == GConstants.m_bucketCount - 1)
        WriteCullDispatch(range);
}
