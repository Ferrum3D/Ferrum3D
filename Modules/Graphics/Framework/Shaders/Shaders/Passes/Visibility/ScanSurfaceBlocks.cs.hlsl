#include <Shaders/Passes/Visibility/Tiling.h>
[[vk::push_constant]] TilingConstants GConstants;

FE_NUM_THREADS(1, 1, 1)
void main()
{
    uint offset = 0;
    for (uint block = 0; block < GConstants.m_pixelCount / 256; ++block)
    {
        GConstants.m_blockOffsets.Store(block, offset);
        offset += GConstants.m_blockTotalsRead.Load(block);
    }
}
