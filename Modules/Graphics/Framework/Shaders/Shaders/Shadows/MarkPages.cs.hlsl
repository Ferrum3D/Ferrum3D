#include <Shaders/Shadows/LocalShadow.hlsli>

[[vk::push_constant]] SunShadowMarkConstants GConstants;

float3 Unproject(const ViewData view, const float2 ndc, const float depth)
{
    const float4 world = mul(float4(ndc, depth, 1), view.m_inverseViewProjection);
    return world.xyz / world.w;
}


void RequestPosition(const SunShadowData data, const float3 position, const float2 pixel, const uint bias)
{
    RWStructuredBuffer<uint> pages = GConstants.m_pageTable.Get();
    SunShadow::RequestPages(pages, data, position, bias);
    LocalShadow::RequestPages(pages, data, GConstants.m_localLighting, GConstants.m_localMaps,
                              GConstants.m_localLightMaps, position, pixel, bias);
}


FE_NUM_THREADS(kSunShadowTileSize, kSunShadowTileSize, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const bool isCoarse = GConstants.m_isCoarse != 0;
    const uint2 pixel = thread.xy * (isCoarse ? 8 : 1);
    if (any(pixel >= GConstants.m_resolution))
        return;

    const ViewData view = GConstants.m_view.Load();
    const SunShadowData data = GConstants.m_data.Load();
    const float2 samplePixel = min(float2(pixel) + (isCoarse ? 4.0f : 0.5f), float2(GConstants.m_resolution) - 0.5f);
    const float2 ndc = (samplePixel - view.m_viewport.xy) / view.m_viewport.zw * float2(2, -2) + float2(-1, 1);
    float depth = GConstants.m_depth.Load(uint3(pixel, 0));
    if (!isCoarse)
    {
        if (depth > 0.0f)
            RequestPosition(data, Unproject(view, ndc, depth), samplePixel, 0);

        return;
    }

    if (any(ndc < GConstants.m_coarseRect.xy) || any(ndc > GConstants.m_coarseRect.zw))
        return;

    // The farthest opaque depth in the tile keeps gaps and silhouette edges covered.
    for (uint y = 0; y < 8; ++y)
    {
        for (uint x = 0; x < 8; ++x)
        {
            const uint2 tap = min(pixel + uint2(x, y), GConstants.m_resolution - 1);
            depth = min(depth, GConstants.m_depth.Load(uint3(tap, 0)));
        }
    }

    const float3 nearPosition = Unproject(view, ndc, 1.0f);
    const float3 endPosition = Unproject(view, ndc, max(depth, 1e-6f));
    const float distance = length(endPosition - nearPosition);
    const float3 direction = (endPosition - nearPosition) / max(distance, 1e-6f);

    // Only the part of the ray between the requested distances from the camera needs coverage.
    const float nearDistance = length(nearPosition - view.m_cameraPosition.xyz);
    const float start = max(GConstants.m_coarseRange.x - nearDistance, 0.0f);
    const float end = min(distance, GConstants.m_coarseRange.y - nearDistance);
    if (end < start)
        return;

    // Slices are spaced geometrically, each at most half as far again as the previous one, which stays within a coarse page.
    const float span = log2((1.0f + end) / (1.0f + start));
    const uint sliceCount = clamp(uint(ceil(span / log2(1.5f))) + 1, 2, 16);
    for (uint slice = 0; slice < sliceCount; ++slice)
    {
        const float offset = (1.0f + start) * exp2(span * float(slice) / float(sliceCount - 1)) - 1.0f;
        RequestPosition(data, nearPosition + direction * offset, samplePixel, 3);
    }
}
