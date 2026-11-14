#include <Shaders/Passes/Fsr3/Fsr3.h>

[[vk::push_constant]] ClearConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint2 pixel : SV_DispatchThreadID)
{
    if (all(pixel < GConstants.m_size))
        GConstants.m_target.Store(pixel, GConstants.m_value);
}
