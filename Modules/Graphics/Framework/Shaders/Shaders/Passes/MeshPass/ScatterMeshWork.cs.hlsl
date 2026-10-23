#include <Shaders/Passes/MeshPass/CullStats.hlsli>
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
    instance.m_lodIndex = item.m_lodIndex;
    instance.m_bucket = bucket;
    instance.m_flags = item.m_flags;

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


void AddStats(const InstanceClassification item, const bool isScattered)
{
    // The late phase revisits instances the early phase rendered to test their remaining meshlets; count those once.
    const bool isLate = GConstants.m_phase == CullPhase::kLate;
    const bool isRenderedEarly = isLate && (item.m_flags & InstanceFlags::kRenderedEarly) != 0;
    const bool isCounted = isScattered && !isRenderedEarly;
    AddCullStat(GConstants.m_stats, CullStat::kInstanceRendered, isCounted);
    AddCullStat(GConstants.m_stats, CullStat::kInstanceRenderedLate, isCounted && isLate);

    // Few distinct LODs are active per wave, so loop over them instead of issuing one atomic per lane.
    bool isPending = isCounted;
    while (WaveActiveAnyTrue(isPending))
    {
        const uint lod = WaveActiveMin(isPending ? item.m_lodIndex : kMaxLodCount);
        const bool matches = isPending && item.m_lodIndex == lod;
        AddCullStat(GConstants.m_stats, CullStat::kLodHistogram + lod, matches);
        isPending = isPending && !matches;
    }
}


FE_NUM_THREADS(kInstancesPerCullGroup, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint index = group.x * kInstancesPerCullGroup + lane;
    const InstanceClassification item = GConstants.m_classification.Load(index);
    const uint bucket = GConstants.m_instanceBuckets.Load(index);
    ScatterBucket(item, bucket);

    if (GConstants.m_stats.IsValid())
        AddStats(item, bucket != kInvalidIndex);
}
