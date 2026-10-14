#include <Shaders/Passes/Visibility/SurfaceClassification.hlsli>

FE_NUM_THREADS(16, 16, 1)
void main(uint3 dispatch : SV_DispatchThreadID)
{
    const uint2 pixel = dispatch.xy;
    const uint bucket = ClassifyPixel(pixel);
    if (bucket == kInvalidIndex && all(pixel < GConstants.m_resolution))
    {
        if (GConstants.m_visibility.Load(uint3(pixel, 0)).x != 0)
            GConstants.m_g0.Store(pixel, 0xffff00ffu);
    }

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
