#include <Shaders/Passes/Visibility/SurfaceClassification.hlsli>

FE_NUM_THREADS(256, 1, 1)
void main(uint3 dispatch : SV_DispatchThreadID)
{
    const uint index = ThreadIndex(dispatch);
    if (index < GConstants.m_scanCount)
        GConstants.m_scanOutput.Store(index, GConstants.m_scanOutput.Load(index) + GConstants.m_parentOffsets.Load(index / 256));
}
