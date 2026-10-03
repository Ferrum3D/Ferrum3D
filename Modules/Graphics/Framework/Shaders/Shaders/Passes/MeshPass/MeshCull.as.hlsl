#include <Shaders/Core/Culling/Culling.hlsli>
#include <Shaders/Passes/MeshPass/Common.hlsli>

groupshared MeshPayload GPayload;
groupshared uint GVisibleCount;

FE_NUM_THREADS(kMeshletsPerWorkChunk, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID)
{
    if (lane == 0)
        GVisibleCount = 0;

    GroupMemoryBarrierWithGroupSync();
    const MeshCommandRange command = GView.m_commands.Load(GConstants.m_commandIndex);
    const uint workIndex = group.x;
    if (workIndex < command.m_workChunkCount)
    {
        const MeshletWorkChunk work = GView.m_workChunks.Load(command.m_firstWorkChunk + workIndex);
        const VisibleInstance instance = GView.m_visibleInstances.Load(work.m_visibleInstanceIndex);
        const MeshDrawData draw = LoadMeshDrawData(instance.m_instanceId, instance.m_lodId);
        const uint meshletIndex = work.m_firstMeshlet + lane;
        if (lane == 0)
        {
            GPayload.m_instanceId = instance.m_instanceId;
            GPayload.m_lodId = instance.m_lodId;
        }

        if (meshletIndex < draw.m_lodInfo.m_meshletCount)
        {
            const uint sphereOffset = draw.m_lodInfo.m_vertexCount * sizeof(VertexInput)
                + draw.m_lodInfo.m_indexCount * sizeof(uint) + draw.m_lodInfo.m_meshletCount * sizeof(Core::MeshletHeader)
                + draw.m_lodInfo.m_primitiveCount * sizeof(Core::PackedTriangle);
            const float4 sphere = draw.m_geometry.Read<float4>(sphereOffset + meshletIndex * sizeof(float4));
            if (VisibleSphere(sphere, mul(draw.m_worldTransform, GView.m_viewProjection)))
            {
                uint payloadIndex;
                InterlockedAdd(GVisibleCount, 1, payloadIndex);
                GPayload.m_meshletIds[payloadIndex] = meshletIndex;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();
    DispatchMesh(GVisibleCount, 1, 1, GPayload);
}
