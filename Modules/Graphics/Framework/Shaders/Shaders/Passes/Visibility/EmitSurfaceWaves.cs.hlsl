#include <Shaders/Passes/Visibility/Tiling.h>
[[vk::push_constant]] TilingConstants GConstants;

FE_NUM_THREADS(256, 1, 1)
void main(uint index : SV_DispatchThreadID)
{
    if (index >= GConstants.m_pixelCount)
        return;

    const uint2 record = GConstants.m_input.Load(index);
    if (record.x == kInvalidIndex)
        return;

    const SurfaceRun run = GConstants.m_runsRead.Load(record.y);
    const uint firstWave = GConstants.m_scanRead.Load(index) + GConstants.m_blockOffsetsRead.Load(index / 256);
    const uint count = (run.m_count + GConstants.m_waveLaneCount - 1) / GConstants.m_waveLaneCount;
    for (uint wave = 0; wave < count; ++wave)
    {
        WaveCommand command;
        command.m_tile = run.m_tile;
        command.m_group = run.m_group;
        command.m_pixelOffset = run.m_pixelOffset + wave * GConstants.m_waveLaneCount;
        command.m_validCount = min(GConstants.m_waveLaneCount, run.m_count - wave * GConstants.m_waveLaneCount);
        GConstants.m_waves.Store(firstWave + wave, command);
    }
}
