#define SURFACE_COUNT
#include <Shaders/Deferred/SurfaceClassification.hlsli>

FE_NUM_THREADS(16, 16, 1)
void main(uint3 dispatch : SV_DispatchThreadID)
{
    const uint2 pixel = dispatch.xy;
    const uint bucket = ClassifyPixel(pixel);
    bool remaining = bucket != kInvalidIndex;
    while (WaveActiveAnyTrue(remaining))
    {
        const uint selectedBucket = WaveActiveMin(remaining ? bucket : kInvalidIndex);
        const bool selected = remaining && bucket == selectedBucket;
        const uint count = WaveActiveCountBits(selected);
        if (WaveIsFirstLane())
            InterlockedAdd(GConstants.m_counts.Get()[selectedBucket], count);

        remaining = remaining && !selected;
    }
}
