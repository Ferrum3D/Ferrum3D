#include <Shaders/Passes/Visibility/Deferred.h>
#include <Shaders/Passes/Visibility/SurfaceCodec.hlsli>
[[vk::push_constant]] DeferredConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (any(pixel.xy >= GConstants.m_resolution))
        return;

    const uint color = GConstants.m_g0.Load(uint3(pixel.xy, 0));
    if ((color >> 24) == 0 || (color >> 24) == 255)
        return;

    const float3 normal = DecodeSurfaceNormal(GConstants.m_g1.Load(uint3(pixel.xy, 0)));
    const float lighting = 0.15f + 0.85f * max(dot(normal, normalize(float3(0.4f, 0.7f, -0.2f))), 0);
    GConstants.m_output.Store(pixel.xy, DecodeSurfaceColor(color) * lighting);
}
