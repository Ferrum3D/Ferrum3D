#include <Shaders/Passes/Visibility/Tiling.h>
[[vk::push_constant]] TilingConstants GConstants;

FE_NUM_THREADS(256, 1, 1)
void main(uint index : SV_DispatchThreadID)
{
    if (index >= GConstants.m_pixelCount)
        return;

    SurfaceRun run = (SurfaceRun)0;
    GConstants.m_output.Store(index, uint2(kInvalidIndex, index));
    const uint2 record = GConstants.m_input.Load(index);
    if (record.x == kInvalidIndex)
    {
        GConstants.m_runs.Store(index, run);
        return;
    }

    const uint tileStart = (index / (kTileSize * kTileSize)) * (kTileSize * kTileSize);
    uint lo = tileStart;
    uint hi = index;
    while (lo < hi)
    {
        const uint mid = (lo + hi) / 2;
        if (GConstants.m_input.Load(mid).x < record.x)
            lo = mid + 1;
        else
            hi = mid;
    }

    const uint first = lo;
    lo = index + 1;
    hi = tileStart + kTileSize * kTileSize;
    while (lo < hi)
    {
        const uint mid = (lo + hi) / 2;
        if (GConstants.m_input.Load(mid).x <= record.x)
            lo = mid + 1;
        else
            hi = mid;
    }

    const uint end = lo;
    const uint relative = index - first;
    if ((relative & 1) == 0)
    {
        const uint second = index + 1 < end ? GConstants.m_input.Load(index + 1).y : 0;
        GConstants.m_pixels.Store(first + relative / 2, record.y | (second << 16));
    }

    if (index == first)
    {
        const ViewData view = GConstants.m_view.Load(0);
        const MeshGroupTable::Row group = MeshGroupTable::Create(view.m_groups).ReadRow(record.x);
        const MaterialInstanceTable::Row material = MaterialInstanceTable::Create(view.m_materials).ReadRow(group.m_materialInstance.Get());
        run.m_group = record.x;
        run.m_tile = GConstants.m_firstTile + index / (kTileSize * kTileSize);
        run.m_pixelOffset = first * 2;
        run.m_count = end - first;
        run.m_evaluationSlot = material.m_evaluationSlot.Get();
        run.m_shadingRoute = material.m_shadingSlot.Get() + 1;
        GConstants.m_output.Store(index, uint2(run.m_evaluationSlot, index));
    }

    GConstants.m_runs.Store(index, run);
}
