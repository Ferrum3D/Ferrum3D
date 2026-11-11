#include <Shaders/Shadows/PageRegions.hlsli>

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
    uint2 pageMin;
    uint2 pageCount;
    if (!ProjectPageRegion(block, regionMin, regionMax, pageMin, pageCount))
        return;

    for (uint pageIndex = lane; pageIndex < pageCount.x * pageCount.y; pageIndex += kSunShadowThreadCount)
    {
        const uint2 page = pageMin + uint2(pageIndex % pageCount.x, pageIndex / pageCount.x);
        const uint index = SunShadow::GetEntryIndex(group.y, SunShadow::GetWorldPage(block.m_pageOrigin, page));
        const uint entry = GConstants.m_pageTable.Load(index);
        if (GConstants.m_marksDynamic != 0)
        {
            SunShadow::RequestEntry(GConstants.m_pageTable.Get(), index, kSunShadowPageDynamic, false);
            continue;
        }

        if ((entry & kSunShadowPageAllocated) == 0)
            continue;

        GConstants.m_owners.Store(entry & kSunShadowPhysicalPageMask, kSunShadowNoOwner);
        GConstants.m_pageTable.Store(index, entry & (kSunShadowPageRequested | kSunShadowPageCoarse));
    }
}
