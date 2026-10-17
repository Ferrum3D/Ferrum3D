#include <Shaders/Deferred/SurfaceClassification.hlsli>

FE_NUM_THREADS(256, 1, 1)
void main(uint3 dispatch : SV_DispatchThreadID)
{
    const uint index = dispatch.x;
    if (index < max(GConstants.m_bucketCount, 1u))
    {
        GConstants.m_counts.Store(index, 0);
        GConstants.m_cursors.Store(index, 0);
    }
}
