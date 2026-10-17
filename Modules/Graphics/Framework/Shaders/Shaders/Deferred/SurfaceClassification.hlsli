#pragma once
#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Deferred/Tiling.h>

#ifdef SURFACE_GROUP_HEADER
#    include SURFACE_GROUP_HEADER
#endif

[[vk::push_constant]] TilingConstants GConstants;

uint ClassifyPixel(uint2 pixel)
{
    if (any(pixel >= GConstants.m_resolution))
        return kInvalidIndex;

    const uint tile = pixel.x / kTileSize + (pixel.y / kTileSize) * GConstants.m_tilesPerRow;
    if (GConstants.m_classifyShading != 0)
    {
        const uint g0 = GConstants.m_g0Read.Load(uint3(pixel, 0));
        const uint route = GBuffer::GetShadingRoute(g0);
        if (route == 0 || route == 255 || route > GConstants.m_shaderCount)
            return kInvalidIndex;

        const SurfaceRoute binding = GConstants.m_lookup.Load(route - 1);
        return binding.m_firstBinding * GConstants.m_tileCount + tile * binding.m_bindingCount + binding.m_bindingIndex;
    }

#ifdef SURFACE_GROUP_HEADER
    const uint2 id = GConstants.m_visibility.Load(uint3(pixel, 0));
    if (id.x == 0)
        return kInvalidIndex;

    const uint group = SurfaceGroup(GConstants.m_view, id);
    if (group >= GConstants.m_groupRowCount)
        return kInvalidIndex;

    const SurfaceRoute route = GConstants.m_lookup.Load(group);
    if (route.m_firstBinding == kInvalidIndex)
        return kInvalidIndex;

    return route.m_firstBinding * GConstants.m_tileCount + tile * route.m_bindingCount + route.m_bindingIndex;
#else
    return kInvalidIndex;
#endif
}
