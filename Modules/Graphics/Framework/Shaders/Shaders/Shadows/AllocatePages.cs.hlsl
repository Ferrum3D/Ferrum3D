#include <Shaders/Shadows/SunShadow.h>

[[vk::push_constant]] SunShadowAllocateConstants GConstants;

// Requested entries without backing take a free atlas page, or else evict one whose entry was not requested this frame.
// Requests beyond the atlas stay unbacked and sample as lit.
FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const uint index = thread.x;
    if (index >= GConstants.m_pageCount)
        return;

    const uint entry = GConstants.m_pageTable.Load(index);
    if ((entry & kSunShadowPageRequested) == 0 || (entry & kSunShadowPageAllocated) != 0)
        return;

    RWStructuredBuffer<uint> counters = GConstants.m_counters.Get();
    uint freeIndex;
    InterlockedAdd(counters[kSunShadowFreeTakenCounter], 1, freeIndex);

    uint physicalPage;
    if (freeIndex < counters[kSunShadowFreeCounter])
    {
        physicalPage = GConstants.m_freePages.Load(freeIndex);
    }
    else
    {
        uint evictableIndex;
        InterlockedAdd(counters[kSunShadowEvictableTakenCounter], 1, evictableIndex);
        if (evictableIndex >= counters[kSunShadowEvictableCounter])
        {
            InterlockedAdd(counters[kSunShadowOverflowCounter], 1);
            return;
        }

        // The evicted entry was not requested, so no other thread of this pass touches it.
        physicalPage = GConstants.m_evictablePages.Load(evictableIndex);
        GConstants.m_pageTable.Store(GConstants.m_owners.Load(physicalPage), 0);
    }

    GConstants.m_owners.Store(physicalPage, index);
    GConstants.m_pageTable.Store(index, entry | kSunShadowPageAllocated | kSunShadowPageDirty | physicalPage);

    uint dirtyIndex;
    InterlockedAdd(counters[kSunShadowAllocatedCounter], 1, dirtyIndex);
    GConstants.m_dirtyPages.Store(dirtyIndex, physicalPage);
}
