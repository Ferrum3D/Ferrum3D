#include <Shaders/Lighting/ForwardTransparency.h>

[[vk::push_constant]] TransparencyResolveConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint2 pixel : SV_DispatchThreadID)
{
    if (any(pixel >= GConstants.m_resolution))
        return;

    const float revealage = GConstants.m_revealage.Load(uint3(pixel, 0));
    const float4 accumulation = GConstants.m_accumulation.Load(uint3(pixel, 0));
    const float3 transparent = accumulation.rgb / max(accumulation.a, 0.00001f);
    const float3 background = GConstants.m_color.Load(pixel);
    GConstants.m_color.Store(pixel, transparent * (1 - revealage) + background * revealage);
}
