#include <Shaders/Shadows/SunShadow.h>

[[vk::push_constant]] SunShadowClearConstants GConstants;

FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint3 thread : SV_DispatchThreadID)
{
    const uint index = thread.x;
    if (index < GConstants.m_pageCount)
        GConstants.m_pageTable.Store(index, 0);

    if (index < kSunShadowCounterCount)
        GConstants.m_counters.Store(index, 0);
}
