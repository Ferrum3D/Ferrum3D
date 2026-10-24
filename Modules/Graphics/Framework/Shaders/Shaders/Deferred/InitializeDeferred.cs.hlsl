#include <Shaders/Deferred/DeferredCommon.hlsli>
#include <Shaders/Lighting/Atmosphere.hlsli>

FE_NUM_THREADS(8, 8, 1)
void main(uint3 dispatch : SV_DispatchThreadID)
{
    const uint2 pixel = dispatch.xy;
    if (any(pixel >= GConstants.m_resolution))
        return;

    const uint g0 = GConstants.m_g0.Load(uint3(pixel, 0));
    const uint route = GBuffer::GetShadingRoute(g0);
    const bool unsupported = route == kUnsupportedShadingRoute || route > GConstants.m_shaderCount;
    if (route == kBackgroundShadingRoute || unsupported)
    {
        const ViewData view = GConstants.m_view.Load(0);
        const float2 ndc = ((float2(pixel) + 0.5f - view.m_viewport.xy) / view.m_viewport.zw) * float2(2, -2) + float2(-1, 1);
        // A finite depth supports infinite-far reversed-Z projections as well.
        const float4 world = mul(float4(ndc, 0.5f, 1), view.m_inverseViewProjection);
        const float3 direction = normalize(world.xyz / world.w - view.m_cameraPosition.xyz);
        const GlobalLightingData lighting = LoadGlobalLighting(GConstants.m_lighting);
        const float3 sky = Atmosphere::SkyLuminance(lighting, view.m_cameraPosition.xyz, direction) * Shading::kPreExposure;
        const float3 color = unsupported ? float3(1, 0, 1) : min(sky, Shading::kMaxStoredLuminance);
        GConstants.m_output.Store(pixel, color);
    }
}
