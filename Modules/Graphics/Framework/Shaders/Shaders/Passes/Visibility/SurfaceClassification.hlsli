#pragma once
#include <Shaders/Passes/Visibility/Tiling.h>
[[vk::push_constant]] TilingConstants GConstants;

uint ThreadIndex(uint3 dispatch)
{
    return dispatch.x + dispatch.y * 65535 * 256;
}


uint ClassifyPixel(uint2 pixel)
{
    if (any(pixel >= GConstants.m_resolution))
        return kInvalidIndex;

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

    const uint tile = pixel.x / kTileSize + (pixel.y / kTileSize) * GConstants.m_tilesPerRow;
    return route.x + tile * route.y;
}
