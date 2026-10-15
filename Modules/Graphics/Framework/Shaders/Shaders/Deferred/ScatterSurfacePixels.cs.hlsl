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
        uint reservation = 0;
        if (WaveIsFirstLane())
            InterlockedAdd(GConstants.m_cursors.Get()[selectedBucket], count, reservation);

        reservation = WaveReadLaneFirst(reservation);
        const uint rank = WavePrefixCountBits(selected);
        if (selected)
        {
            const uint destination = GConstants.m_offsets.Load(selectedBucket).x + reservation + rank;
            GConstants.m_pixels.Store(destination, (pixel.x & 127) | ((pixel.y & 127) << 7));
        }

        remaining = remaining && !selected;
    }
}
