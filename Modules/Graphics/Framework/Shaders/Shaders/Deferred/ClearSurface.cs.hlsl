#include <Shaders/Deferred/Surface.h>

[[vk::push_constant]] SurfaceConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (all(pixel.xy < GConstants.m_resolution))
    {
        const uint instance = GConstants.m_visibility.Load(uint3(pixel.xy, 0)).x;
        GConstants.m_g0.Store(pixel.xy, instance == 0 ? 0 : 0xffff00ffu);
    }
}
