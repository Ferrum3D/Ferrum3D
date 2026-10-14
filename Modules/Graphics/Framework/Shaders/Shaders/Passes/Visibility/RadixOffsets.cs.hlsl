#include <Shaders/Passes/Visibility/Tiling.h>
[[vk::push_constant]] TilingConstants GConstants;
groupshared uint GTotals[16];

FE_NUM_THREADS(16, 1, 1)
void main(uint bin : SV_GroupThreadID, uint3 segment : SV_GroupID)
{
    const uint blockCount = GConstants.m_segmented != 0 ? 64 : GConstants.m_pixelCount / 256;
    const uint firstBlock = segment.x * blockCount;
    uint total = 0;
    for (uint block = 0; block < blockCount; ++block)
        total += GConstants.m_histogramRead.Load((firstBlock + block) * 16 + bin);

    GTotals[bin] = total;
    GroupMemoryBarrierWithGroupSync();
    uint offset = firstBlock * 256;
    for (uint previous = 0; previous < bin; ++previous)
        offset += GTotals[previous];

    for (uint block = 0; block < blockCount; ++block)
    {
        const uint address = (firstBlock + block) * 16 + bin;
        GConstants.m_offsets.Store(address, offset);
        offset += GConstants.m_histogramRead.Load(address);
    }
}
