#pragma once
#include <Shaders/Deferred/Tiling.h>

#ifndef EVALUATION_LANES
#    define EVALUATION_LANES 32
#endif

// A logical 64-lane command can span two NVIDIA waves. A larger hardware wave may
// contain multiple 32-lane commands, in which case these values must remain varying.
template<typename T>
T SurfaceWaveUniform(T value)
{
    return WaveGetLaneCount() <= EVALUATION_LANES ? WaveReadLaneFirst(value) : value;
}


ShaderRange LoadSurfaceRange(StructuredBufferDescriptor<ShaderRange> ranges, uint shaderSlot)
{
    ShaderRange range = ranges.Load(shaderSlot);
    range.m_firstWave = WaveReadLaneFirst(range.m_firstWave);
    range.m_waveCount = WaveReadLaneFirst(range.m_waveCount);
    return range;
}


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
    const uint waveIndex = SurfaceWaveUniform(wave);
    if (waveIndex >= range.m_waveCount)
        return false;

    result.m_command = waves.Load(range.m_firstWave + waveIndex);
    result.m_command.m_tile = SurfaceWaveUniform(result.m_command.m_tile);
    result.m_command.m_group = SurfaceWaveUniform(result.m_command.m_group);
    result.m_command.m_pixelOffset = SurfaceWaveUniform(result.m_command.m_pixelOffset);
    result.m_command.m_validCount = SurfaceWaveUniform(result.m_command.m_validCount);
    if (lane >= result.m_command.m_validCount)
        return false;

    const uint local = pixels.Load(result.m_command.m_pixelOffset + lane);
    const uint tileIndex = result.m_command.m_tile;
    const uint2 tile = uint2(tileIndex % tilesPerRow, tileIndex / tilesPerRow);
    const uint2 localPixel = uint2(local & 127, (local >> 7) & 127);
    result.m_pixel = tile * kTileSize + localPixel;
    return all(result.m_pixel < resolution);
}
