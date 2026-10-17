#include <Shaders/Deferred/SurfaceClassification.hlsli>

groupshared uint2 GScan[256];

FE_NUM_THREADS(256, 1, 1)
void main(uint3 dispatch : SV_DispatchThreadID, uint lane : SV_GroupIndex)
{
    const uint index = dispatch.x;
    uint2 value = 0;
    if (index < GConstants.m_scanCount)
    {
        if (GConstants.m_scanBuckets != 0)
        {
            const uint count = index < GConstants.m_bucketCount ? GConstants.m_countsRead.Load(index) : 0;
            value = uint2(count, (count + GConstants.m_waveLaneCount - 1) / GConstants.m_waveLaneCount);
        }
        else
            value = GConstants.m_scanInput.Load(index);
    }

    GScan[lane] = value;
    GroupMemoryBarrierWithGroupSync();
    for (uint step = 1; step < 256; step *= 2)
    {
        const uint2 previous = lane >= step ? GScan[lane - step] : 0;
        GroupMemoryBarrierWithGroupSync();
        GScan[lane] += previous;
        GroupMemoryBarrierWithGroupSync();
    }

    if (index < GConstants.m_scanCount)
        GConstants.m_scanOutput.Store(index, GScan[lane] - value);

    if (lane == 255 && index / 256 < (GConstants.m_scanCount + 255) / 256)
        GConstants.m_scanSums.Store(index / 256, GScan[lane]);
}
