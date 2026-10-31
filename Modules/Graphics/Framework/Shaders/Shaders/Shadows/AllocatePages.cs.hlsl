#include <Shaders/Shadows/SunShadow.h>

[[vk::push_constant]] SunShadowAllocateConstants GConstants;

// Back every requested page with the next free atlas page. Requests beyond the atlas stay unbacked and sample as lit.
FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const uint index = thread.x;
    if (index >= GConstants.m_pageCount)
        return;

    const uint entry = GConstants.m_pageTable.Load(index);
    if ((entry & kSunShadowPageRequested) == 0)
        return;

    RWStructuredBuffer<uint> counters = GConstants.m_counters.Get();
    uint physicalPage;
    InterlockedAdd(counters[kSunShadowAllocatedCounter], 1, physicalPage);
    if (physicalPage >= kSunShadowPhysicalPageCount)
    {
        InterlockedAdd(counters[kSunShadowOverflowCounter], 1);
        return;
    }

    GConstants.m_pageTable.Store(index, entry | kSunShadowPageAllocated | physicalPage);
}
