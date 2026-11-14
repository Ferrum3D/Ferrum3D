#include <Shaders/Tests/Fsr3Probe.h>

[[vk::push_constant]] FillConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint2 pixel : SV_DispatchThreadID)
{
    if (any(pixel >= GConstants.m_size))
        return;

    GConstants.m_color.Store(pixel, GConstants.m_value);
    GConstants.m_depth.Store(pixel, 0.5f);
    GConstants.m_motion.Store(pixel, 0);
    GConstants.m_reactive.Store(pixel, 0.5f);
}
