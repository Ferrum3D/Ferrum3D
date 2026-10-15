#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Tests/DeferredProbe.h>
[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 dispatch : SV_DispatchThreadID)
{
    const uint2 pixel = dispatch.xy;
    if (any(pixel >= GConstants.m_resolution))
        return;

    const uint index = pixel.x + pixel.y * GConstants.m_resolution.x;
    uint route = index % 5 == 0 ? 3 : 1;
    if (GConstants.m_empty != 0 || index % 7 == 0)
        route = 0;
    else if (index % 11 == 0)
        route = 255;
    else if (index % 13 == 0)
        route = 4;

    GBuffer::Unpacked surface = (GBuffer::Unpacked)0;
    surface.m_color = float3(0.4f, 0.6f, 0.8f);
    surface.m_normal = float3(0, 0, -1);
    surface.m_roughness = 140.0f / 255.0f;
    surface.m_metallic = 64.0f / 255.0f;
    surface.m_ambientOcclusion = 34.0f / 255.0f;
    surface.m_emissive = (index & 1) != 0 ? float3(32, 64, 128) : 0;
    surface.m_shadingRoute = route;
    const GBuffer::Packed packed = GBuffer::Encode(surface);
    GConstants.m_g0.Store(pixel, packed.m_g0);
    // Unused planes deliberately contain invalid/poison data on background and unsupported pixels.
    GConstants.m_g1.Store(pixel, route == 1 || route == 3 ? packed.m_g1 : 0xffffffffu);
    GConstants.m_g2.Store(pixel, packed.m_g2);
    GConstants.m_g3.Store(pixel, (index & 1) != 0 ? packed.m_g3 : 0xffffffffu);
    GConstants.m_depth.Store(pixel, 0.2f + 0.6f * float(pixel.x) / float(GConstants.m_resolution.x));
    if (index < 4)
        GConstants.m_stats.Store(index, 0);
}
