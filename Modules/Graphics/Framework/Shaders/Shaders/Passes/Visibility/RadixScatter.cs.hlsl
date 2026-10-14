#include <Shaders/Passes/Visibility/Tiling.h>
[[vk::push_constant]] TilingConstants GConstants;
groupshared uint2 GRecords[256];

FE_NUM_THREADS(256, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 block : SV_GroupID)
{
    GRecords[lane] = GConstants.m_input.Load(block.x * 256 + lane);
    GroupMemoryBarrierWithGroupSync();
    if (lane < 16)
    {
        uint offset = GConstants.m_offsetsRead.Load(block.x * 16 + lane);
        for (uint pixel = 0; pixel < 256; ++pixel)
        {
            const uint2 record = GRecords[pixel];
            if (((record.x >> GConstants.m_shift) & 15) == lane)
                GConstants.m_output.Store(offset++, record);
        }
    }
}
