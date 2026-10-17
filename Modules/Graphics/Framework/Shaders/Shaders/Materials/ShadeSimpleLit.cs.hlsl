#include <Shaders/Deferred/DeferredCommon.hlsli>

FE_NUM_THREADS(EVALUATION_LANES * kSurfaceWavesPerGroup, 1, 1)
void main(uint3 dispatch : SV_GroupID, uint lane : SV_GroupIndex)
{
    uint2 pixel;
    if (!LoadShadingPixel(dispatch, lane, pixel))
        return;

    GBuffer::Packed packed = (GBuffer::Packed)0;
    const uint3 location = uint3(pixel, 0);
    packed.m_g0 = GConstants.m_g0.Load(location);
    const uint route = GBuffer::GetShadingRoute(packed.m_g0);
    if (route == kBackgroundShadingRoute || route == kUnsupportedShadingRoute)
        return;

    packed.m_g1 = GConstants.m_g1.Load(location);
    const GBuffer::Unpacked surface = GBuffer::Decode(packed);
    const float3 lightDirection = normalize(float3(0.4f, 0.7f, -0.2f));
    const float nDotL = max(dot(surface.m_normal, lightDirection), 0);
    const float lighting = 0.15f + 0.85f * nDotL;
    GConstants.m_output.Store(pixel, surface.m_color * lighting);
}
