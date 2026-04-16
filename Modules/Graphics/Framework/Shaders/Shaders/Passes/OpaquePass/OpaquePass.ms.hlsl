#include <Shaders/Passes/MeshPass/Common.hlsli>

FE_NUM_THREADS(64, 1, 1)
FE_OUTPUT_TOPOLOGY("triangle")
void main(const in uint32_t groupThreadID : SV_GroupThreadID,
          const in uint32_t groupID : SV_GroupID,
          out ms_vertices PixelAttributes verts[64],
          out ms_indices uint3 tris[64])
{
    const MeshDrawData drawData = LoadMeshDrawData();
    if (groupID >= drawData.m_lodInfo.m_meshletCount)
        return;

    const uint32_t indicesByteOffset = drawData.m_lodInfo.m_vertexCount * sizeof(VertexInput);
    const uint32_t meshletsByteOffset = indicesByteOffset + drawData.m_lodInfo.m_indexCount * sizeof(uint32_t);
    const uint32_t primitivesByteOffset =
        meshletsByteOffset + drawData.m_lodInfo.m_meshletCount * sizeof(Core::MeshletHeader);

    const Core::MeshletHeader meshlet =
        drawData.m_geometry.Read<Core::MeshletHeader>(groupID * sizeof(Core::MeshletHeader) + meshletsByteOffset);
    SetMeshOutputCounts(meshlet.m_vertexCount, meshlet.m_primitiveCount);

    if (groupThreadID < meshlet.m_vertexCount)
    {
        const uint32_t index =
            drawData.m_geometry.Read<uint32_t>((meshlet.m_vertexOffset + groupThreadID) * sizeof(uint32_t) + indicesByteOffset);
        PixelAttributes attributes = LoadAttributes(drawData, index);
        attributes.m_meshletIndex = groupID;
        verts[groupThreadID] = attributes;
    }

    if (groupThreadID < meshlet.m_primitiveCount)
        tris[groupThreadID] = LoadPrimitive(drawData, meshlet.m_primitiveOffset + groupThreadID, primitivesByteOffset);
}
