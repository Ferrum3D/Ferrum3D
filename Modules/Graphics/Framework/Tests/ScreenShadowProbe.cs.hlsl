#include <Shaders/Tests/ScreenShadowProbe.h>

[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(kThreadGroupSize, kThreadGroupSize, 1)
void main(uint3 texel : SV_DispatchThreadID)
{
    if (any(texel.xy >= GConstants.m_size))
        return;

    if (GConstants.m_mode == kFillMode)
    {
        const bool isSlab = all(texel.xy >= GConstants.m_slabMin) && all(texel.xy < GConstants.m_slabMax);
        GConstants.m_target.Store(texel.xy, isSlab ? kSlabDepth : kFloorDepth);
        return;
    }

    const float value = GConstants.m_source.Load(uint3(texel.xy, 0));
    GConstants.m_output.Store(texel.y * GConstants.m_size.x + texel.x, value);
}
