#include <Shaders/Tests/SurfaceProbe.h>
#ifndef VISIT_LANES
#    define VISIT_LANES 32
#endif
#ifndef VISIT_SHADER_COUNT
#    define VISIT_SHADER_COUNT 3
#endif
#ifndef VISIT_SHADER_STRIDE
#    define VISIT_SHADER_STRIDE 1
#endif

#define EVALUATION_LANES VISIT_LANES
#include <Shaders/Deferred/PixelWork.hlsli>

[[vk::push_constant]] ProbeConstants GConstants;

FE_NUM_THREADS(VISIT_LANES * kSurfaceWavesPerGroup, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 dispatch : SV_GroupID)
{
    const ShaderRange range = LoadSurfaceRange(GConstants.m_ranges, GConstants.m_shaderSlot);
    const uint wave = dispatch.x * kSurfaceWavesPerGroup + lane / VISIT_LANES;
    const uint waveLane = lane % VISIT_LANES;
    const uint tilesPerRow = (GConstants.m_resolution.x + kTileSize - 1) / kTileSize;
    PixelWork work;
    if (!LoadPixelWork(range,
                       GConstants.m_waves,
                       GConstants.m_pixels,
                       GConstants.m_resolution,
                       tilesPerRow,
                       wave,
                       waveLane,
                       work))
    {
        return;
    }

    const WaveCommand command = work.m_command;
    const uint offset = command.m_pixelOffset + waveLane;
    const uint local = GConstants.m_pixels.Load(offset);
    const uint2 pixel = work.m_pixel;
    const uint index = pixel.x + pixel.y * GConstants.m_resolution.x;
    InterlockedAdd(GConstants.m_marks.Get()[index * 2], 1);
#if VISIT_SHADING
    const bool wrongGroup = command.m_group != GConstants.m_shaderSlot;
#else
    const bool wrongGroup = command.m_group != GConstants.m_groupBase + index % GConstants.m_groupCount;
#endif
    const bool wrongShader =
        ((index % GConstants.m_groupCount) % VISIT_SHADER_COUNT) * VISIT_SHADER_STRIDE != GConstants.m_shaderSlot;
    if (wrongGroup || wrongShader || (local & 0xffffc000u) != 0 || command.m_validCount > VISIT_LANES)
        GConstants.m_marks.Store(index * 2 + 1, 1);
}
