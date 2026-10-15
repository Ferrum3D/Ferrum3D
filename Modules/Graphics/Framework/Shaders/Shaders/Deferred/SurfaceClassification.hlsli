#pragma once
#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Deferred/Tiling.h>

[[vk::push_constant]] TilingConstants GConstants;

uint ThreadIndex(uint3 dispatch)
{
    return dispatch.x + dispatch.y * 65535 * 256;
}


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

        const uint2 binding = GConstants.m_lookup.Load(route - 1);
        return binding.x + tile * binding.y;
    }

    const uint2 id = GConstants.m_visibility.Load(uint3(pixel, 0));
    if (id.x == 0)
        return kInvalidIndex;

    const ViewData view = GConstants.m_view.Load(0);
    const MeshInstanceTable::Row instance = MeshInstanceTable::Create(view.m_instances).ReadRow(id.x - 1);
    const uint group = instance.m_meshGroup.Get().m_rowIndex;
    if (group >= GConstants.m_groupRowCount)
        return kInvalidIndex;

    const uint2 route = GConstants.m_lookup.Load(group);
    if (route.x == kInvalidIndex)
        return kInvalidIndex;

    return route.x + tile * route.y;
}
