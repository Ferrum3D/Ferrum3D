#include <Shaders/Passes/MeshPass/FinalizeMeshBuckets.h>

[[vk::push_constant]] Constants GConstants;

void BuildBucketCommands(uint bucket, PipelineBucket range)
{
    const BucketLayout layout = GConstants.m_layouts.Load(bucket);
    for (uint page = 0; page < layout.m_commandCount; ++page)
    {
        const uint offset = page * kWorkChunksPerCommand;
        const uint count = offset < range.m_workChunkCount ? min(kWorkChunksPerCommand, range.m_workChunkCount - offset) : 0;

        MeshCommandRange command;
        command.m_firstWorkChunk = range.m_firstWorkChunk + min(offset, range.m_workChunkCount);
        command.m_workChunkCount = count;
        GConstants.m_commands.Store(layout.m_firstCommand + page, command);

        MeshDispatchArguments args;
        args.m_groupCountX = count;
        args.m_groupCountY = count == 0 ? 0 : 1;
        args.m_groupCountZ = count == 0 ? 0 : 1;
        GConstants.m_arguments.Store(layout.m_firstCommand + page, args);
    }
}

FE_NUM_THREADS(kThreadCount, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    const uint bucket = group.x * kThreadCount + lane;
    if (bucket >= GConstants.m_bucketCount)
        return;

    const uint2 offset = GConstants.m_offsets.Load(bucket);
    PipelineBucket range;
    range.m_firstInstance = offset.x;
    range.m_firstWorkChunk = offset.y;
    range.m_instanceCount = GConstants.m_counts.Load(bucket * 2);
    range.m_workChunkCount = GConstants.m_counts.Load(bucket * 2 + 1);
    GConstants.m_buckets.Store(bucket, range);
    BuildBucketCommands(bucket, range);
}
