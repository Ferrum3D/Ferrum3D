#include <Shaders/Editor/Selection.h>

[[vk::push_constant]] SelectionConstants GConstants;

bool Selected(int2 pixel)
{
    if (any(pixel < 0) || any(pixel >= int2(GConstants.m_resolution)))
        return false;

    const uint object = GConstants.m_visibility.Load(int3(pixel, 0)).x;
    for (uint i = 0; i < GConstants.m_selectedCount; ++i)
    {
        if (object == GConstants.m_selected.Load(i))
            return true;
    }

    return false;
}

FE_NUM_THREADS(8, 8, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const uint2 pixel = thread.xy;
    if (any(pixel >= GConstants.m_resolution))
        return;

    if (GConstants.m_pick != 0 && all(pixel == GConstants.m_pickPosition))
        GConstants.m_pickResult.Store(0, GConstants.m_visibility.Load(int3(pixel, 0)).x);

    if (GConstants.m_selectedCount == 0)
        return;

    const int2 p = int2(pixel);
    const bool selected = Selected(p);
    const bool edge = Selected(p + int2(2, 0)) != selected || Selected(p - int2(2, 0)) != selected
        || Selected(p + int2(0, 2)) != selected || Selected(p - int2(0, 2)) != selected;
    if (edge)
        GConstants.m_color.Store(pixel, float3(4.0, 1.5, 0.05));
}
