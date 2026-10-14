#include <Shaders/Passes/Visibility/Tiling.h>
[[vk::push_constant]] TilingConstants GConstants;
groupshared uint GScan[256];

FE_NUM_THREADS(256, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 block : SV_GroupID)
{
    const uint index = block.x * 256 + lane;
    const uint2 record = GConstants.m_input.Load(index);
    const uint count = record.x != kInvalidIndex ? GConstants.m_runsRead.Load(record.y).m_count : 0;
    const uint waves = (count + GConstants.m_waveLaneCount - 1) / GConstants.m_waveLaneCount;
    GScan[lane] = waves;
    GroupMemoryBarrierWithGroupSync();
    for (uint step = 1; step < 256; step *= 2)
    {
        const uint previous = lane >= step ? GScan[lane - step] : 0;
        GroupMemoryBarrierWithGroupSync();
        GScan[lane] += previous;
        GroupMemoryBarrierWithGroupSync();
    }

    GConstants.m_scan.Store(index, GScan[lane] - waves);
    if (lane == 255)
        GConstants.m_blockTotals.Store(block.x, GScan[lane]);
}
