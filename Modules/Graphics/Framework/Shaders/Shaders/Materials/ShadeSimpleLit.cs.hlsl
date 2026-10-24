#include <Shaders/Deferred/DeferredCommon.hlsli>
#include <Shaders/Lighting/Atmosphere.hlsli>

FE_NUM_THREADS(EVALUATION_LANES* kSurfaceWavesPerGroup, 1, 1)
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
    const GlobalLightingData lighting = LoadGlobalLighting(GConstants.m_lighting);
    const ViewData view = GConstants.m_view.Load(0);
    const float2 ndc = ((float2(pixel) + 0.5f - view.m_viewport.xy) / view.m_viewport.zw) * float2(2, -2) + float2(-1, 1);
    const float depth = GConstants.m_depth.Load(location);
    const float4 world = mul(float4(ndc, depth, 1), view.m_inverseViewProjection);
    const float nDotL = saturate(dot(surface.m_normal, lighting.m_directionIlluminance.xyz));
    const float3 illuminance = Atmosphere::DirectionalIlluminance(lighting, world.xyz / world.w);
    GConstants.m_output.Store(pixel, surface.m_color * illuminance * (nDotL * Shading::kPreExposure / Atmosphere::kPi));
}
