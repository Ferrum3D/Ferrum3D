#pragma once
#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Deferred/Tiling.h>

#ifdef SURFACE_GROUP_HEADER
#    include SURFACE_GROUP_HEADER
#    ifdef SURFACE_COUNT
[[vk::push_constant]] EvaluationCountConstants GConstants;
#    else
[[vk::push_constant]] EvaluationScatterConstants GConstants;
#    endif
#else
#    ifdef SURFACE_COUNT
[[vk::push_constant]] ShadingCountConstants GConstants;
#    else
[[vk::push_constant]] ShadingScatterConstants GConstants;
#    endif
#endif


uint ClassifyPixel(uint2 pixel)
{
    if (any(pixel >= GConstants.m_resolution))
        return kInvalidIndex;

#ifdef SURFACE_GROUP_HEADER
    const uint2 id = GConstants.m_visibility.Load(uint3(pixel, 0));
    if (id.x == 0)
        return kInvalidIndex;

    const uint group = SurfaceGroup(GConstants.m_view, id);
#else
    const uint g0 = GConstants.m_g0Read.Load(uint3(pixel, 0));
    const uint shadingRoute = GBuffer::GetShadingRoute(g0);
    if (shadingRoute == kBackgroundShadingRoute || shadingRoute == kUnsupportedShadingRoute)
        return kInvalidIndex;

    const uint group = shadingRoute - 1;
#endif
    if (group >= GConstants.m_groupRowCount)
        return kInvalidIndex;

    const SurfaceRoute route = GConstants.m_lookup.Load(group);
    if (route.m_firstBinding == kInvalidIndex)
        return kInvalidIndex;

    const uint tile = pixel.x / kTileSize + pixel.y / kTileSize * GConstants.m_tilesPerRow;
    return route.m_firstBinding * GConstants.m_tileCount + tile * route.m_bindingCount + route.m_bindingIndex;
}
