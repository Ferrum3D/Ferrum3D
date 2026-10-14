#include <Shaders/Tests/SurfaceProbe.h>
[[vk::push_constant]] ProbeConstants GConstants;
#ifndef VISIT_LANES
#define VISIT_LANES 32
#endif

FE_NUM_THREADS(VISIT_LANES, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 dispatch : SV_GroupID)
{
    const ShaderRange range = GConstants.m_ranges.Load(GConstants.m_shaderSlot);
    const uint wave = dispatch.x + dispatch.y * 65535;
    if (wave >= range.m_waveCount)
        return;

    const WaveCommand command = GConstants.m_waves.Load(range.m_firstWave + wave);
    if (lane >= command.m_validCount)
        return;

    const uint offset = command.m_pixelOffset + lane;
    const uint packed = GConstants.m_pixels.Load(offset / 2);
    const uint local = (packed >> ((offset & 1) * 16)) & 65535;
    const uint tilesPerRow = (GConstants.m_resolution.x + 127) / 128;
    const uint2 pixel = uint2(command.m_tile % tilesPerRow, command.m_tile / tilesPerRow) * 128
        + uint2(local & 127, (local >> 7) & 127);
    if (any(pixel >= GConstants.m_resolution))
        return;

    const uint index = pixel.x + pixel.y * GConstants.m_resolution.x;
    InterlockedAdd(GConstants.m_marks.Get()[index * 2], 1);
    const bool wrongGroup = command.m_group != GConstants.m_groupBase + index % GConstants.m_groupCount;
    const bool wrongShader = (index % GConstants.m_groupCount) % 3 != GConstants.m_shaderSlot;
    if (wrongGroup || wrongShader || (local & 49152) != 0 || command.m_validCount > VISIT_LANES)
        GConstants.m_marks.Store(index * 2 + 1, 1);
}
