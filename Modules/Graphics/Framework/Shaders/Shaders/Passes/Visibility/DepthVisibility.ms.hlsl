#include <Shaders/Passes/MeshPass/Common.hlsli>
#include <Shaders/Passes/Visibility/Contracts.h>

struct CoverageVertex
{
    float4 m_position : SV_Position;
};


struct CoveragePrimitive
{
    nointerpolation uint2 m_visibility : TEXCOORD0;
};


FE_NUM_THREADS(kMeshShaderThreadCount, 1, 1)
FE_OUTPUT_TOPOLOGY("triangle")
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID, in ms_payload MeshPayload payloadData,
          out ms_vertices CoverageVertex verts[64], out ms_indices uint3 tris[64],
          out ms_primitives CoveragePrimitive prims[64])
{
    const uint meshletIndex = payloadData.m_meshletIds[group.x];
    const MeshDrawData draw = LoadMeshDrawData(payloadData.m_instanceId, payloadData.m_lodId);
    const uint remapOffset = draw.m_lodInfo.m_vertexCount * sizeof(VertexInput);
    const uint headerOffset = remapOffset + draw.m_lodInfo.m_indexCount * sizeof(uint);
    const uint triangleOffset = headerOffset + draw.m_lodInfo.m_meshletCount * sizeof(Core::MeshletHeader);
    const Core::MeshletHeader header = draw.m_geometry.Read<Core::MeshletHeader>(headerOffset + meshletIndex * sizeof(Core::MeshletHeader));
    SetMeshOutputCounts(header.m_vertexCount, header.m_primitiveCount);

    if (lane < header.m_vertexCount)
    {
        const uint vertexIndex = draw.m_geometry.Read<uint>(remapOffset + (header.m_vertexOffset + lane) * sizeof(uint));
        const float3 position = draw.m_geometry.Read<float3>(vertexIndex * sizeof(VertexInput));
        const float4 worldPosition = mul(float4(position, 1.0f), draw.m_worldTransform);
        verts[lane].m_position = mul(worldPosition, GView.m_viewProjection);
    }

    if (lane < header.m_primitiveCount)
    {
        tris[lane] = LoadPrimitive(draw, header.m_primitiveOffset + lane, triangleOffset);
        prims[lane].m_visibility = uint2(payloadData.m_instanceId + 1, (meshletIndex << kPrimitiveBits) | lane);
    }
}
