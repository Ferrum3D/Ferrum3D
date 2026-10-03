#include <Shaders/Passes/MeshPass/ScanMeshBuckets.h>

[[vk::push_constant]] Constants GConstants;

groupshared uint2 GScan[kThreadCount];

FE_NUM_THREADS(kThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint block = group.x;
    const uint index = block * kThreadCount + lane;
    const uint2 value = index < GConstants.m_count ? GConstants.m_input.Load(index) : uint2(0, 0);
    GScan[lane] = value;
    GroupMemoryBarrierWithGroupSync();

    for (uint offset = 1; offset < kThreadCount; offset *= 2)
    {
        const uint2 previous = lane >= offset ? GScan[lane - offset] : uint2(0, 0);
        GroupMemoryBarrierWithGroupSync();
        GScan[lane] += previous;
        GroupMemoryBarrierWithGroupSync();
    }

    if (index < GConstants.m_count)
        GConstants.m_output.Store(index, GScan[lane] - value);

    if (lane == kThreadCount - 1 && block < (GConstants.m_count + kThreadCount - 1) / kThreadCount)
        GConstants.m_sums.Store(block, GScan[lane]);
}
