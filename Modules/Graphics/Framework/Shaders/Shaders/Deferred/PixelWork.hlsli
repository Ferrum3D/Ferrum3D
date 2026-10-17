#pragma once
#include <Shaders/Deferred/Tiling.h>

struct PixelWork
{
    uint2 m_pixel;
    WaveCommand m_command;
};


// Evaluation and shading use the same compact tile-local pixel list and wave commands.
bool LoadPixelWork(const ShaderRange range, StructuredBufferDescriptor<WaveCommand> waves,
                   StructuredBufferDescriptor<uint> pixels, const uint2 resolution, const uint tilesPerRow, const uint wave,
                   const uint lane, out PixelWork result)
{
    result = (PixelWork)0;
    if (wave >= range.m_waveCount)
        return false;

    result.m_command = waves.Load(range.m_firstWave + wave);
    if (lane >= result.m_command.m_validCount)
        return false;

    const uint local = pixels.Load(result.m_command.m_pixelOffset + lane);
    const uint tileIndex = result.m_command.m_tile;
    const uint2 tile = uint2(tileIndex % tilesPerRow, tileIndex / tilesPerRow);
    const uint2 localPixel = uint2(local & 127, (local >> 7) & 127);
    result.m_pixel = tile * kTileSize + localPixel;
    return all(result.m_pixel < resolution);
}
