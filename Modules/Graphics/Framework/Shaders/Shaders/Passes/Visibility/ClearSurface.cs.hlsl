#include <Shaders/Passes/Visibility/Surface.h>
[[vk::push_constant]] SurfaceConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (all(pixel.xy < GConstants.m_resolution))
        GConstants.m_g0.Store(pixel.xy, 0);
}
