#include <Shaders/Tests/Fsr3Probe.h>

[[vk::push_constant]] InspectConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint2 pixel : SV_DispatchThreadID)
{
    if (all(pixel < GConstants.m_size))
        GConstants.m_pixels.Store(pixel.y * GConstants.m_size.x + pixel.x, GConstants.m_color.Load(uint3(pixel, 0)));
}
