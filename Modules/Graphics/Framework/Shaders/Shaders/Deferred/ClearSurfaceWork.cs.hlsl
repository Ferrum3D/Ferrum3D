#include <Shaders/Deferred/SurfaceClassification.hlsli>

FE_NUM_THREADS(256, 1, 1)
void main(uint3 dispatch : SV_DispatchThreadID)
{
    const uint index = ThreadIndex(dispatch);
    if (index < max(GConstants.m_bucketCount, 1u))
    {
        GConstants.m_counts.Store(index, 0);
        GConstants.m_cursors.Store(index, 0);
    }

    if (GConstants.m_classifyShading == 0 && index < GConstants.m_resolution.x * GConstants.m_resolution.y)
        GConstants.m_g0.Store(uint2(index % GConstants.m_resolution.x, index / GConstants.m_resolution.x), 0);
}
