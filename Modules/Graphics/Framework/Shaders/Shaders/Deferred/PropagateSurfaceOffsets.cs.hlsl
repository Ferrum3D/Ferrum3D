#include <Shaders/Deferred/Tiling.h>

[[vk::push_constant]] SurfacePropagateConstants GConstants;

FE_NUM_THREADS(256, 1, 1)
void main(uint3 dispatch : SV_DispatchThreadID)
{
    const uint index = dispatch.x;
    if (index < GConstants.m_scanCount)
        GConstants.m_scanOutput.Store(index, GConstants.m_scanOutput.Load(index) + GConstants.m_parentOffsets.Load(index / 256));
}
