#include <Shaders/Passes/MeshPass/FinalizeMeshletCommands.h>

[[vk::push_constant]] Constants GConstants;

// Every reserved command page is written, so pages beyond the visible meshlets dispatch nothing.
FE_NUM_THREADS(kThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint bucket = group.x * kThreadCount + lane;
    if (bucket >= GConstants.m_bucketCount)
        return;

    const uint firstMeshlet = GConstants.m_buckets.Load(bucket).m_firstWorkChunk * kMeshletsPerWorkChunk;
    const uint meshletCount = GConstants.m_meshletCounts.Load(bucket);
    const BucketLayout layout = GConstants.m_layouts.Load(bucket);
    for (uint page = 0; page < layout.m_commandCount; ++page)
    {
        const uint offset = page * kMeshletsPerCommand;
        const uint count = offset < meshletCount ? min(kMeshletsPerCommand, meshletCount - offset) : 0;

        MeshCommandRange command;
        command.m_firstMeshlet = firstMeshlet + min(offset, meshletCount);
        command.m_meshletCount = count;
        GConstants.m_commands.Store(layout.m_firstCommand + page, command);

        MeshDispatchArguments args;
        args.m_groupCountX = count;
        args.m_groupCountY = count == 0 ? 0 : 1;
        args.m_groupCountZ = count == 0 ? 0 : 1;
        GConstants.m_arguments.Store(layout.m_firstCommand + page, args);
    }
}
