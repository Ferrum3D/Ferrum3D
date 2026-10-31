#include <Shaders/Shadows/SunShadow.hlsli>

[[vk::push_constant]] SunShadowUpdateConstants GConstants;

// The world page an entry addresses while its level starts at the origin page.
int2 GetEntryWorldPage(const uint2 slot, const int2 pageOrigin)
{
    const int2 levelOrigin = int2(pageOrigin.x, -pageOrigin.y);
    return levelOrigin + ((int2(slot) - levelOrigin) & int2(kSunShadowLevelPagesPerAxis - 1, kSunShadowLevelPagesPerAxis - 1));
}


FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const uint index = thread.x;
    if (index < kSunShadowCounterCount)
        GConstants.m_counters.Store(index, 0);

    if (GConstants.m_isReset != 0 && index < kSunShadowPhysicalPageCount)
        GConstants.m_owners.Store(index, kSunShadowNoOwner);

    if (index >= GConstants.m_pageCount)
        return;

    if (GConstants.m_isReset != 0)
    {
        GConstants.m_pageTable.Store(index, 0);
        return;
    }

    // Entries whose world page left the level when it scrolled release their atlas page.
    const uint entry = GConstants.m_pageTable.Load(index);
    const uint level = index / kSunShadowLevelPageCount;
    const uint slotIndex = index % kSunShadowLevelPageCount;
    const uint2 slot = uint2(slotIndex % kSunShadowLevelPagesPerAxis, slotIndex / kSunShadowLevelPagesPerAxis);
    const SunShadowScroll scroll = GConstants.m_scroll.Load(level);
    const bool isMoved = any(GetEntryWorldPage(slot, scroll.m_previousOrigin) != GetEntryWorldPage(slot, scroll.m_currentOrigin));
    if (isMoved)
    {
        if ((entry & kSunShadowPageAllocated) != 0)
            GConstants.m_owners.Store(entry & kSunShadowPhysicalPageMask, kSunShadowNoOwner);

        GConstants.m_pageTable.Store(index, 0);
        return;
    }

    GConstants.m_pageTable.Store(index, entry & ~(kSunShadowPageRequested | kSunShadowPageDirty));
}
