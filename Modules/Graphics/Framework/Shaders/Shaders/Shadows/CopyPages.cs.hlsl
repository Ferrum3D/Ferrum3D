#include <Shaders/Shadows/SunShadow.h>

[[vk::push_constant]] SunShadowCopyPagesConstants GConstants;

static const uint kTexelsPerThread = kSunShadowPageSize * kSunShadowPageSize / kSunShadowThreadCount;

// One group per atlas page copies the static depth of requested entries; dynamic casters are added on top.
FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint physicalPage = group.x;
    const uint owner = GConstants.m_owners.Load(physicalPage);
    if (owner == kSunShadowNoOwner || (GConstants.m_pageTable.Load(owner) & kSunShadowPageRequested) == 0)
        return;

    const uint2 pageOrigin = uint2(physicalPage % kSunShadowAtlasPagesPerAxis, physicalPage / kSunShadowAtlasPagesPerAxis);
    for (uint texelIndex = 0; texelIndex < kTexelsPerThread; ++texelIndex)
    {
        const uint linearIndex = texelIndex * kSunShadowThreadCount + lane;
        const uint2 inPage = uint2(linearIndex % kSunShadowPageSize, linearIndex / kSunShadowPageSize);
        const uint2 texel = pageOrigin * kSunShadowPageSize + inPage;
        GConstants.m_target.Store(texel, GConstants.m_source.Load(uint3(texel, 0)));
    }
}
