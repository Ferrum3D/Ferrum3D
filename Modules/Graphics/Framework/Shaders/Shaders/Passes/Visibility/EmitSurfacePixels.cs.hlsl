#include <Shaders/Passes/Visibility/Tiling.h>
[[vk::push_constant]] TilingConstants GConstants;

FE_NUM_THREADS(256, 1, 1)
void main(uint index : SV_DispatchThreadID)
{
    if (index >= GConstants.m_pixelCount)
        return;

    const uint localTile = index / (kTileSize * kTileSize);
    const uint tile = GConstants.m_firstTile + localTile;
    const uint within = index % (kTileSize * kTileSize);
    const uint2 local = uint2(within % kTileSize, within / kTileSize);
    const uint2 pixel = uint2(tile % GConstants.m_tilesPerRow, tile / GConstants.m_tilesPerRow) * kTileSize + local;
    uint groupIndex = kInvalidIndex;
    if (all(pixel < GConstants.m_resolution))
    {
        const uint2 id = GConstants.m_visibility.Load(uint3(pixel, 0));
        if (id.x != 0)
        {
            const ViewData view = GConstants.m_view.Load(0);
            const MeshInstanceTable::Row instance = MeshInstanceTable::Create(view.m_instances).ReadRow(id.x - 1);
            const MeshGroupTable::Row group = MeshGroupTable::Create(view.m_groups).ReadRow(instance.m_meshGroup.Get());
            const MaterialInstanceTable::Row material = MaterialInstanceTable::Create(view.m_materials).ReadRow(group.m_materialInstance.Get());
            if (material.m_evaluationSlot.Get() < GConstants.m_shaderCount && material.m_shadingSlot.Get() < 254)
                groupIndex = instance.m_meshGroup.Get().m_rowIndex;
            else
                GConstants.m_g0.Store(pixel, 0xffff00ffu);
        }
    }

    GConstants.m_output.Store(index, uint2(groupIndex, local.x | (local.y << 7)));
}
