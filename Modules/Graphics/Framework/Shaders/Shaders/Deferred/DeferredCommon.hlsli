#pragma once
#include <Shaders/Deferred/Deferred.h>
#include <Shaders/Deferred/GBuffer.hlsli>

#ifndef EVALUATION_LANES
#    define EVALUATION_LANES 32
#endif

[[vk::push_constant]] DeferredConstants GConstants;

bool LoadShadingPixel(uint3 dispatch, uint lane, out uint2 pixel)
{
    pixel = 0;
    const ShaderRange range = GConstants.m_ranges.Load(GConstants.m_shaderSlot);
    const uint index = dispatch.x + dispatch.y * 65535;
    if (index >= range.m_waveCount)
        return false;

    const WaveCommand command = GConstants.m_waves.Load(range.m_firstWave + index);
    if (lane >= command.m_validCount)
        return false;

    const uint local = GConstants.m_pixels.Load(command.m_pixelOffset + lane);
    const uint2 tile = uint2(command.m_tile % GConstants.m_tilesPerRow, command.m_tile / GConstants.m_tilesPerRow);
    const uint2 localPixel = uint2(local & 127, (local >> 7) & 127);
    pixel = tile * kTileSize + localPixel;
    return all(pixel < GConstants.m_resolution);
}


GBuffer::Unpacked LoadGBuffer(const uint2 pixel)
{
    const uint3 location = uint3(pixel, 0);
    GBuffer::Packed packed;
    packed.m_g0 = GConstants.m_g0.Load(location);
    packed.m_g1 = GConstants.m_g1.Load(location);
    packed.m_g2 = GConstants.m_g2.Load(location);
    packed.m_g3 = 0;
    if (GBuffer::HasExtendedPayload(packed.m_g2))
        packed.m_g3 = GConstants.m_g3.Load(location);

    return GBuffer::Decode(packed);
}
