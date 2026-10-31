#include <Shaders/Tests/SunShadowProbe.h>

[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(kThreadGroupSize, kThreadGroupSize, 1)
void main(uint3 texel : SV_DispatchThreadID)
{
    if (any(texel.xy >= GConstants.m_size))
        return;

    GConstants.m_target.Store(texel.xy, texel.y < GConstants.m_backgroundRows ? 0.0f : GConstants.m_depth);
}
