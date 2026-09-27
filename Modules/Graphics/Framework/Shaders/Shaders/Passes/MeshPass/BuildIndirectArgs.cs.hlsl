#include <Shaders/Passes/MeshPass/BuildIndirectArgs.h>

[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(64, 1, 1)
void main(const uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const uint groupIndex = dispatchThreadID.x;
    if (groupIndex >= GConstants.m_groupCount)
        return;

    const MeshDispatchArguments counts = GConstants.m_groupCounts.Load(groupIndex);
    GConstants.m_arguments.Store(groupIndex, counts);
}
