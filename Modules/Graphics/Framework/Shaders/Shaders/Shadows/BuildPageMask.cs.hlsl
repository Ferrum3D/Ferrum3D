#include <Shaders/Shadows/SunShadow.hlsli>

[[vk::push_constant]] SunShadowPageMaskConstants GConstants;
groupshared float Mask[kSunShadowLevelPageCount];

FE_NUM_THREADS(kSunShadowThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const SunShadowPageMaskTarget target = GConstants.m_targets.LoadElement(group.x);
    uint size = target.m_pagesPerAxis;
    for (uint index = lane; index < size * size; index += kSunShadowThreadCount)
    {
        const uint2 page = uint2(index % size, index / size);
        const uint entry = GConstants.m_pageTable.Load(SunShadow::GetPageIndex(target.m_level, target.m_pageOrigin,
                                                                              page * kSunShadowPageSize));
        const float value = (entry & target.m_requiredFlags) == target.m_requiredFlags ? 0.0f : 1.0f;
        Mask[index] = value;
        target.m_mips[0].Store(page, value);
    }

    GroupMemoryBarrierWithGroupSync();
    for (uint mip = 1; size > 1; ++mip)
    {
        const uint nextSize = size / 2;
        // Each iteration reads a disjoint 2x2 block before anyone overwrites LDS.
        float values[4];
        for (uint item = 0; item < 4; ++item)
        {
            const uint index = lane + item * kSunShadowThreadCount;
            float value = 1.0f;
            if (index < nextSize * nextSize)
            {
                const uint source = (index / nextSize * 2) * size + index % nextSize * 2;
                value = min(min(Mask[source], Mask[source + 1]), min(Mask[source + size], Mask[source + size + 1]));
                target.m_mips[mip].Store(uint2(index % nextSize, index / nextSize), value);
            }

            values[item] = value;
        }

        GroupMemoryBarrierWithGroupSync();
        for (uint item = 0; item < 4; ++item)
        {
            const uint index = lane + item * kSunShadowThreadCount;
            if (index < nextSize * nextSize)
                Mask[index] = values[item];
        }

        GroupMemoryBarrierWithGroupSync();
        size = nextSize;
    }
}
