#include <Shaders/Shadows/SunShadow.hlsli>

[[vk::push_constant]] SunShadowMarkConstants GConstants;

// Every visible pixel requests the pages its filter footprint reads.
FE_NUM_THREADS(kSunShadowTileSize, kSunShadowTileSize, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const uint2 pixel = thread.xy;
    if (any(pixel >= GConstants.m_resolution))
        return;

    const float depth = GConstants.m_depth.Load(uint3(pixel, 0));
    if (depth <= 0.0f)
        return;

    const ViewData view = GConstants.m_view.Load(0);
    const float2 viewportPosition = (float2(pixel) + 0.5f - view.m_viewport.xy) / view.m_viewport.zw;
    const float2 ndc = viewportPosition * float2(2, -2) + float2(-1, 1);
    const float4 homogeneousWorld = mul(float4(ndc, depth, 1), view.m_inverseViewProjection);
    const float3 worldPosition = homogeneousWorld.xyz / homogeneousWorld.w;

    SunShadow::RequestPages(GConstants.m_pageTable.Get(), GConstants.m_data.Load(0), worldPosition);
}
