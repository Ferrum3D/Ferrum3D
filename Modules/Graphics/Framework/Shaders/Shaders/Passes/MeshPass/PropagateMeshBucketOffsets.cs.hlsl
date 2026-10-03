#include <Shaders/Passes/MeshPass/PropagateMeshBucketOffsets.h>

[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(kThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint index = group.x * kThreadCount + lane;
    if (index >= GConstants.m_count)
        return;

    const uint2 offset = GConstants.m_output.Load(index) + GConstants.m_input.Load(index / kBucketsPerScanGroup);
    GConstants.m_output.Store(index, offset);
}
