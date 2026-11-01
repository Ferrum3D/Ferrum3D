#include <Shaders/Shadows/SunShadow.h>

[[vk::push_constant]] SunShadowAllocateConstants GConstants;

// Takes a free atlas page, or else evicts one whose entry was not requested this frame. Requests beyond the atlas stay
// unbacked and sample as lit.
uint AllocateEntry(RWStructuredBuffer<uint> counters, const uint index, const uint entry)
{
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
            return entry;
        }

        // The evicted entry was not requested, so no other thread of this pass touches it.
        physicalPage = GConstants.m_evictablePages.Load(evictableIndex);
        GConstants.m_pageTable.Store(GConstants.m_owners.Load(physicalPage), 0);
    }

    const uint allocated = entry | kSunShadowPageAllocated | kSunShadowPageDirty | physicalPage;
    GConstants.m_owners.Store(physicalPage, index);
    GConstants.m_pageTable.Store(index, allocated);

    uint dirtyIndex;
    InterlockedAdd(counters[kSunShadowAllocatedCounter], 1, dirtyIndex);
    GConstants.m_dirtyPages.Store(dirtyIndex, physicalPage);
    return allocated;
}


// Requested entries without backing are backed, and every block counts its requested and dirty pages.
FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const uint index = thread.x;
    if (index >= GConstants.m_pageCount)
        return;

    RWStructuredBuffer<uint> counters = GConstants.m_counters.Get();
    const uint entry = GConstants.m_pageTable.Load(index);
    const bool isRequested = (entry & kSunShadowPageRequested) != 0;
    const bool needsBacking = isRequested && (entry & kSunShadowPageAllocated) == 0;
    const uint result = needsBacking ? AllocateEntry(counters, index, entry) : entry;

    // Groups cover a quarter of a block, so the entries of a wave belong to one block.
    const uint block = index / kSunShadowLevelPageCount;
    const bool isDirty = (result & (kSunShadowPageAllocated | kSunShadowPageDirty)) == (kSunShadowPageAllocated | kSunShadowPageDirty);
    const uint requestCount = WaveActiveCountBits(isRequested);
    const uint dirtyCount = WaveActiveCountBits(isDirty);
    if (WaveIsFirstLane() && requestCount != 0)
        InterlockedAdd(counters[kSunShadowBlockRequestCounter + block], requestCount);
    if (WaveIsFirstLane() && dirtyCount != 0)
        InterlockedAdd(counters[kSunShadowBlockDirtyCounter + block], dirtyCount);
}
