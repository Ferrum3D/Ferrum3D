#include <Shaders/Shadows/SunShadow.h>

[[vk::push_constant]] SunShadowClearAtlasConstants GConstants;

static const uint kTexelsPerThread = kSunShadowPageSize * kSunShadowPageSize / kSunShadowThreadCount;

// One group per atlas page; groups beyond the allocated pages exit. Zero is the farthest reverse-Z depth.
FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint physicalPage = group.x;
    if (physicalPage >= min(GConstants.m_counters.Load(kSunShadowAllocatedCounter), kSunShadowPhysicalPageCount))
        return;

    const uint2 pageOrigin = uint2(physicalPage % kSunShadowAtlasPagesPerAxis, physicalPage / kSunShadowAtlasPagesPerAxis);
    for (uint texelIndex = 0; texelIndex < kTexelsPerThread; ++texelIndex)
    {
        const uint linearIndex = texelIndex * kSunShadowThreadCount + lane;
        const uint2 texel = uint2(linearIndex % kSunShadowPageSize, linearIndex / kSunShadowPageSize);
        GConstants.m_atlas.Store(pageOrigin * kSunShadowPageSize + texel, 0);
    }
}
