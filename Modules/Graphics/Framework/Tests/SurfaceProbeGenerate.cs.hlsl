#include <Shaders/Tests/SurfaceProbe.h>
[[vk::push_constant]] ProbeConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (any(pixel.xy >= GConstants.m_resolution))
        return;

    const uint index = pixel.x + pixel.y * GConstants.m_resolution.x;
    const uint instance = GConstants.m_instanceBase + index % max(GConstants.m_groupCount, 1u);
    GConstants.m_visibility.Store(pixel.xy, uint2(GConstants.m_groupCount == 0 || index % 7 == 0 ? 0 : instance + 1, 0));
    GConstants.m_g0.Store(pixel.xy, 0);
    GConstants.m_marks.Store(index * 2, 0);
    GConstants.m_marks.Store(index * 2 + 1, 0);
}
