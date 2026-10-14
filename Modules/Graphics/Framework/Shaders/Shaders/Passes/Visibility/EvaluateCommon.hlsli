#pragma once
#include <Shaders/Passes/Visibility/Surface.h>
#include <Shaders/Passes/Visibility/Reconstruction.hlsli>
#include <Shaders/Passes/Visibility/SurfaceCodec.hlsli>

#ifndef EVALUATION_LANES
#define EVALUATION_LANES 32
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
    const uint packed = GConstants.m_pixels.Load(offset / 2);
    const uint local = (packed >> ((offset & 1) * 16)) & 65535;
    pixel = uint2(command.m_tile % GConstants.m_tilesPerRow, command.m_tile / GConstants.m_tilesPerRow) * kTileSize
        + uint2(local & 127, (local >> 7) & 127);
    groupIndex = command.m_group;
    return all(pixel < GConstants.m_resolution);
}


uint SurfaceShadingRoute(ViewData view, uint groupIndex)
{
    const MeshGroupTable::Row group = MeshGroupTable::Create(view.m_groups).ReadRow(groupIndex);
    const MaterialInstanceTable::Row material = MaterialInstanceTable::Create(view.m_materials).ReadRow(group.m_materialInstance.Get());
    return material.m_shadingSlot.Get() < 254 ? material.m_shadingSlot.Get() + 1 : kUnsupportedShadingRoute;
}


void StoreSurface(uint2 pixel, float3 color, float3 normal, float roughness, float metallic, float ao, float3 emissive, uint route)
{
    emissive = max(select(isfinite(emissive), emissive, 0.0f), 0.0f);
    const uint flags = any(emissive > 0) ? kExtendedPayloadValid : 0;
    GConstants.m_g0.Store(pixel, EncodeSurfaceColor(color, route));
    GConstants.m_g1.Store(pixel, EncodeSurfaceNormal(normal));
    GConstants.m_g2.Store(pixel, EncodeSurfaceProperties(roughness, metallic, ao, flags));
    if (flags != 0)
        GConstants.m_g3.Store(pixel, EncodeSurfaceEmissive(emissive));
}
