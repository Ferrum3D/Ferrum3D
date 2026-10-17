#include <Shaders/Tests/SurfaceProbe.h>
[[vk::push_constant]] ProbeConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (any(pixel.xy >= GConstants.m_resolution))
        return;

    const uint index = pixel.x + pixel.y * GConstants.m_resolution.x;
    const uint instance = GConstants.m_instanceBase + index % max(GConstants.m_groupCount, 1u);
    bool covered = GConstants.m_groupCount != 0 && index % 7 != 0;
#if GENERATE_SPARSE
    covered = covered && index % 101 == 0;
#endif
#if GENERATE_TAIL
    covered = covered && index < 5;
#endif
    GConstants.m_visibility.Store(pixel.xy, uint2(covered ? instance + 1 : 0, 0));
#if GENERATE_SHADING
    uint route = (index % max(GConstants.m_groupCount, 1u)) % 3 * 8 + 1;
    if (GConstants.m_groupCount == 0 || index % 7 == 0)
        route = 0;
    else if (index % 11 == 0)
        route = 255;

    GConstants.m_g0.Store(pixel.xy, route << 24);
#else
    GConstants.m_g0.Store(pixel.xy, 0);
#endif
    GConstants.m_marks.Store(index * 2, 0);
    GConstants.m_marks.Store(index * 2 + 1, 0);
}
