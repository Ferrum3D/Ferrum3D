#pragma once
#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Deferred/Surface.h>
#include <Shaders/Passes/Visibility/Reconstruction.hlsli>

#ifndef EVALUATION_LANES
#    define EVALUATION_LANES 32
#endif

[[vk::push_constant]] SurfaceConstants GConstants;

bool LoadEvaluationPixel(uint3 dispatch, uint lane, out uint2 pixel, out uint groupIndex)
{
    pixel = 0;
    groupIndex = kInvalidIndex;
    const ShaderRange range = GConstants.m_ranges.Load(GConstants.m_shaderSlot);
    const uint wave = dispatch.x + dispatch.y * 65535;
    if (wave >= range.m_waveCount)
        return false;

    const WaveCommand command = GConstants.m_waves.Load(range.m_firstWave + wave);
    if (lane >= command.m_validCount)
        return false;

    const uint offset = command.m_pixelOffset + lane;
    const uint local = GConstants.m_pixels.Load(offset);
    const uint2 tile = uint2(command.m_tile % GConstants.m_tilesPerRow, command.m_tile / GConstants.m_tilesPerRow);
    const uint2 localPixel = uint2(local & 127, (local >> 7) & 127);
    pixel = tile * kTileSize + localPixel;
    groupIndex = command.m_group;
    return all(pixel < GConstants.m_resolution);
}


uint SurfaceShadingRoute(ViewData view, uint groupIndex)
{
    const MeshGroupTable::Row group = MeshGroupTable::Create(view.m_groups).ReadRow(groupIndex);
    const MaterialInstanceTable::Row material =
        MaterialInstanceTable::Create(view.m_materials).ReadRow(group.m_materialInstance.Get());
    const uint slot = material.m_shadingSlot.Get();
    return slot < 254 ? slot + 1 : kUnsupportedShadingRoute;
}


void StoreGBuffer(const uint2 pixel, const GBuffer::Unpacked surface)
{
    const GBuffer::Packed packed = GBuffer::Encode(surface);
    GConstants.m_g0.Store(pixel, packed.m_g0);
    GConstants.m_g1.Store(pixel, packed.m_g1);
    GConstants.m_g2.Store(pixel, packed.m_g2);
    if (GBuffer::HasExtendedPayload(packed.m_g2))
        GConstants.m_g3.Store(pixel, packed.m_g3);
}
