#include <Shaders/Shadows/SunShadow.h>

[[vk::push_constant]] SunShadowCollectConstants GConstants;

uint AppendIndex(const uint counter)
{
    RWStructuredBuffer<uint> counters = GConstants.m_counters.Get();
    uint listIndex;
    InterlockedAdd(counters[counter], 1, listIndex);
    return listIndex;
}


FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const uint physicalPage = thread.x;
    if (physicalPage >= kSunShadowPhysicalPageCount)
        return;

    const uint owner = GConstants.m_owners.Load(physicalPage);
    if (owner == kSunShadowNoOwner)
    {
        GConstants.m_freePages.Store(AppendIndex(kSunShadowFreeCounter), physicalPage);
        return;
    }

    if ((GConstants.m_pageTable.Load(owner) & kSunShadowPageRequested) == 0)
        GConstants.m_evictablePages.Store(AppendIndex(kSunShadowEvictableCounter), physicalPage);
}
