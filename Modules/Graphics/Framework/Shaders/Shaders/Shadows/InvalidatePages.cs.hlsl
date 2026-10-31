#include <Shaders/Shadows/SunShadow.hlsli>

[[vk::push_constant]] SunShadowInvalidateConstants GConstants;

// One group per region and level frees the pages its bounds cover. Groups freeing the same page store the same values.
FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint level = group.y;
    const SunShadowData data = GConstants.m_data.Load(0);
    if (level >= data.m_levelCount)
        return;

    const float3 regionMin = GConstants.m_regions.Load(group.x * 2).xyz;
    const float3 regionMax = GConstants.m_regions.Load(group.x * 2 + 1).xyz;
    float2 texelMin = 1e30f;
    float2 texelMax = -1e30f;
    [unroll] for (uint corner = 0; corner < 8; ++corner)
    {
        const float3 position = float3((corner & 1) ? regionMax.x : regionMin.x,
                                       (corner & 2) ? regionMax.y : regionMin.y,
                                       (corner & 4) ? regionMax.z : regionMin.z);
        float depth;
        const float2 texel = SunShadow::ProjectTexel(data, level, position, depth);
        texelMin = min(texelMin, texel);
        texelMax = max(texelMax, texel);
    }

    if (any(texelMax < 0.0f) || any(texelMin >= kSunShadowVirtualSize))
        return;

    const uint2 pageMin = uint2(clamp(floor(texelMin / kSunShadowPageSize), 0.0f, kSunShadowLevelPagesPerAxis - 1.0f));
    const uint2 pageMax = uint2(clamp(floor(texelMax / kSunShadowPageSize), 0.0f, kSunShadowLevelPagesPerAxis - 1.0f));
    const uint2 pageCount = pageMax - pageMin + 1;
    const int2 pageOrigin = data.m_levels[level].m_pageOrigin;
    for (uint pageIndex = lane; pageIndex < pageCount.x * pageCount.y; pageIndex += kSunShadowThreadCount)
    {
        const uint2 page = pageMin + uint2(pageIndex % pageCount.x, pageIndex / pageCount.x);
        const uint index = SunShadow::GetEntryIndex(level, SunShadow::GetWorldPage(pageOrigin, page));
        const uint entry = GConstants.m_pageTable.Load(index);
        if ((entry & kSunShadowPageAllocated) == 0)
            continue;

        GConstants.m_owners.Store(entry & kSunShadowPhysicalPageMask, kSunShadowNoOwner);
        GConstants.m_pageTable.Store(index, 0);
    }
}
