#include <Shaders/Passes/Visibility/DebugCommon.hlsli>
#include <Shaders/Tests/DeferredProbe.h>

[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (any(pixel.xy >= GConstants.m_resolution))
        return;

    const float3 color = GConstants.m_color.Load(uint3(pixel.xy, 0));
    const bool wrong = !all(isfinite(color)) || any(abs(color - kDebugBackgroundColor) > 0.001f);
    GConstants.m_stats.Store(pixel.x + pixel.y * GConstants.m_resolution.x, wrong ? 1 : 0);
}
