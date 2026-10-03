#include <Shaders/Passes/MeshPass/ClearMeshWork.h>

[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(kThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint index = group.x * kThreadCount + lane;
    if (index < GConstants.m_bucketCount * 2)
    {
        GConstants.m_counts.Store(index, 0);
        GConstants.m_cursors.Store(index, 0);
    }
}
