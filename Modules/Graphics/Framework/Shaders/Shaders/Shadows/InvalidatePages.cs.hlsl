#include <Shaders/Shadows/SunShadow.hlsli>

[[vk::push_constant]] SunShadowInvalidateConstants GConstants;

// One group per region and block frees the pages its bounds cover. Groups freeing the same page store the same values.
FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const SunShadowBlock block = GConstants.m_blocks.Load(group.y);
    if (block.m_pagesPerAxis == 0)
        return;

    const float3 regionMin = GConstants.m_regions.Load(group.x * 2).xyz;
    const float3 regionMax = GConstants.m_regions.Load(group.x * 2 + 1).xyz;
    const float virtualSize = float(block.m_pagesPerAxis * kSunShadowPageSize);
    float2 texelMin = 1e30f;
    float2 texelMax = -1e30f;
    bool isBehind = false;
    [unroll] for (uint corner = 0; corner < 8; ++corner)
    {
        const float3 position = float3((corner & 1) ? regionMax.x : regionMin.x,
                                       (corner & 2) ? regionMax.y : regionMin.y,
                                       (corner & 4) ? regionMax.z : regionMin.z);
        const float4 clip = mul(float4(position, 1.0f), block.m_viewProjection);
        isBehind = isBehind || clip.w <= 0.0f;
        const float2 texel = (clip.xy / max(clip.w, 1e-6f) * float2(0.5f, -0.5f) + 0.5f) * virtualSize;
        texelMin = min(texelMin, texel);
        texelMax = max(texelMax, texel);
    }

    // Bounds that reach behind a perspective map cover an unbounded part of it, so the whole map is freed.
    if (isBehind && block.m_isPerspective != 0)
    {
        texelMin = 0.0f;
        texelMax = virtualSize - 1.0f;
    }

    if (any(texelMax < 0.0f) || any(texelMin >= virtualSize))
        return;

    const float lastPage = float(block.m_pagesPerAxis - 1);
    const uint2 pageMin = uint2(clamp(floor(texelMin / kSunShadowPageSize), 0.0f, lastPage));
    const uint2 pageMax = uint2(clamp(floor(texelMax / kSunShadowPageSize), 0.0f, lastPage));
    const uint2 pageCount = pageMax - pageMin + 1;
    for (uint pageIndex = lane; pageIndex < pageCount.x * pageCount.y; pageIndex += kSunShadowThreadCount)
    {
        const uint2 page = pageMin + uint2(pageIndex % pageCount.x, pageIndex / pageCount.x);
        const uint index = SunShadow::GetEntryIndex(group.y, SunShadow::GetWorldPage(block.m_pageOrigin, page));
        const uint entry = GConstants.m_pageTable.Load(index);
        if ((entry & kSunShadowPageAllocated) == 0)
            continue;

        GConstants.m_owners.Store(entry & kSunShadowPhysicalPageMask, kSunShadowNoOwner);
        GConstants.m_pageTable.Store(index, 0);
    }
}
