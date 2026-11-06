#pragma once
#include <Shaders/Lighting/LocalLighting.h>

//! First mask word of the light cluster that contains the position seen at the pixel.
uint GetLightClusterBase(const LightClusterData data, const float3 position, const float2 pixel)
{
    const float z = mul(float4(position, 1), data.m_view).z;
    const uint slice =
        (uint)clamp(floor(log2(max(z, data.m_depth.x) / data.m_depth.x) * data.m_depth.z), 0.0f, float(kLightClusterSlices - 1));
    const uint2 tile = min(uint2(max(pixel - data.m_viewData.Load().m_viewport.xy, 0)) / kLightClusterTileSize, data.m_tiles - 1);
    return ((slice * data.m_tiles.y + tile.y) * data.m_tiles.x + tile.x) * data.m_wordCount;
}
