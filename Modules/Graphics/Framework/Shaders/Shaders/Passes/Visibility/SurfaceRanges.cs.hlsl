#include <Shaders/Passes/Visibility/Tiling.h>
[[vk::push_constant]] TilingConstants GConstants;

uint WaveOffset(uint index)
{
    if (index == GConstants.m_pixelCount)
    {
        const uint block = GConstants.m_pixelCount / 256 - 1;
        return GConstants.m_blockOffsetsRead.Load(block) + GConstants.m_blockTotalsRead.Load(block);
    }

    return GConstants.m_scanRead.Load(index) + GConstants.m_blockOffsetsRead.Load(index / 256);
}

FE_NUM_THREADS(256, 1, 1)
void main(uint slot : SV_DispatchThreadID)
{
    if (slot >= GConstants.m_shaderCount)
        return;

    uint lo = 0;
    uint hi = GConstants.m_pixelCount;
    while (lo < hi)
    {
        const uint mid = (lo + hi) / 2;
        if (GConstants.m_input.Load(mid).x < slot)
            lo = mid + 1;
        else
            hi = mid;
    }

    const uint first = lo;
    hi = GConstants.m_pixelCount;
    while (lo < hi)
    {
        const uint mid = (lo + hi) / 2;
        if (GConstants.m_input.Load(mid).x <= slot)
            lo = mid + 1;
        else
            hi = mid;
    }

    ShaderRange range;
    range.m_firstWave = WaveOffset(first);
    range.m_waveCount = WaveOffset(lo) - range.m_firstWave;
    GConstants.m_ranges.Store(slot, range);
    DispatchArguments arguments;
    arguments.m_x = min(range.m_waveCount, 65535u);
    arguments.m_y = range.m_waveCount == 0 ? 1 : (range.m_waveCount + 65534) / 65535;
    arguments.m_z = 1;
    GConstants.m_arguments.Store(slot, arguments);
}
