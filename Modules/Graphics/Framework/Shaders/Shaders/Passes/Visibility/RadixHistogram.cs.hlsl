#include <Shaders/Passes/Visibility/Tiling.h>
[[vk::push_constant]] TilingConstants GConstants;
groupshared uint GDigits[256];

FE_NUM_THREADS(256, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 block : SV_GroupID)
{
    const uint index = block.x * 256 + lane;
    GDigits[lane] = (GConstants.m_input.Load(index).x >> GConstants.m_shift) & 15;
    GroupMemoryBarrierWithGroupSync();
    if (lane < 16)
    {
        uint count = 0;
        for (uint pixel = 0; pixel < 256; ++pixel)
            count += GDigits[pixel] == lane;

        GConstants.m_histogram.Store(block.x * 16 + lane, count);
    }
}
