#include <Shaders/Shadows/SunShadow.hlsli>

[[vk::push_constant]] SunShadowMarkConstants GConstants;

void RequestPage(const uint level, const int2 texel)
{
    const uint2 clamped = uint2(clamp(texel, 0, (int)kSunShadowVirtualSize - 1));
    GConstants.m_pageTable.Store(SunShadow::GetPageIndex(level, clamped), kSunShadowPageRequested);
}


// Every visible pixel requests the pages its filter footprint reads. All writers store the same value, so plain stores suffice.
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

    const SunShadowData data = GConstants.m_data.Load(0);
    const SunShadow::Projection projection = SunShadow::Project(data, worldPosition);
    if (!projection.m_isValid)
        return;

    // Sampling offsets the receiver by a few texels and filters a 3x3 footprint, so request the pages around it.
    const int2 texel = int2(floor(projection.m_texel));
    RequestPage(projection.m_level, texel + int2(-3, -3));
    RequestPage(projection.m_level, texel + int2(3, -3));
    RequestPage(projection.m_level, texel + int2(-3, 3));
    RequestPage(projection.m_level, texel + int2(3, 3));
}
