#include <Shaders/Shadows/SunShadow.hlsli>

[[vk::push_constant]] SunShadowUpdateConstants GConstants;
groupshared uint Feedback[kSunShadowThreadCount / 32];

// The world page an entry addresses while its level starts at the origin page.
int2 GetEntryWorldPage(const uint2 slot, const int2 pageOrigin)
{
    const int2 levelOrigin = int2(pageOrigin.x, -pageOrigin.y);
    return levelOrigin + ((int2(slot) - levelOrigin) & int2(kSunShadowLevelPagesPerAxis - 1, kSunShadowLevelPagesPerAxis - 1));
}


FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint3 thread : SV_DispatchThreadID, uint lane : SV_GroupThreadID)
{
    const uint index = thread.x;
    if (lane < kSunShadowThreadCount / 32)
    {
        const uint word = (index - lane) / 32 + lane;
        Feedback[lane] = GConstants.m_isReset != 0 ? 0 : GConstants.m_feedback.Load(word);
        GConstants.m_feedback.Store(word, 0);
    }

    GroupMemoryBarrierWithGroupSync();
    const bool wasRequested = (Feedback[lane / 32] & (1u << (index % 32))) != 0;
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

    // Entries whose world page left the block when it scrolled, and the entries of discarded blocks, release their atlas page.
    const uint entry = GConstants.m_pageTable.Load(index);
    const uint block = index / kSunShadowLevelPageCount;
    const uint slotIndex = index % kSunShadowLevelPageCount;
    const uint2 slot = uint2(slotIndex % kSunShadowLevelPagesPerAxis, slotIndex / kSunShadowLevelPagesPerAxis);
    const SunShadowScroll scroll = GConstants.m_scroll.Load(block);
    const bool isMoved = any(GetEntryWorldPage(slot, scroll.m_previousOrigin) != GetEntryWorldPage(slot, scroll.m_currentOrigin));
    if (isMoved || scroll.m_isReset != 0)
    {
        if ((entry & kSunShadowPageAllocated) != 0)
            GConstants.m_owners.Store(entry & kSunShadowPhysicalPageMask, kSunShadowNoOwner);

        GConstants.m_pageTable.Store(index, 0);
        return;
    }

    const uint clearedFlags = kSunShadowPageRequested | kSunShadowPageDynamic | kSunShadowPageCoarse | kSunShadowPageAdmitted
        | kSunShadowPageUndrawn | (scroll.m_keepsDirty != 0 ? 0 : kSunShadowPageDirty);
    GConstants.m_pageTable.Store(index, (entry & ~clearedFlags) | (wasRequested ? kSunShadowPageRequested : 0));
}
