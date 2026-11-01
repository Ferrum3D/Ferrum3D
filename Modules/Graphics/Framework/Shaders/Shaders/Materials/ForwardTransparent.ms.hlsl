#include <Shaders/Passes/MeshPass/Common.hlsli>


FE_NUM_THREADS(kMeshShaderThreadCount, 1, 1)
FE_OUTPUT_TOPOLOGY("triangle")
void main(uint lane : SV_GroupThreadID, uint3 group : SV_GroupID, out ms_vertices PixelAttributes verts[64],
          out ms_indices uint3 tris[64])
{
    const VisibleMeshlet visibleMeshlet = LoadVisibleMeshlet(group.x);
    const uint lodIndex = visibleMeshlet.m_lodMeshlet >> kVisibleMeshletLodShift;
    const uint meshletIndex = visibleMeshlet.m_lodMeshlet & ((1u << kVisibleMeshletLodShift) - 1);
    const MeshDrawData draw = LoadMeshDrawData(visibleMeshlet.m_instanceId, lodIndex);
    const MeshletLayout layout = GetMeshletLayout(draw.m_lodInfo);
    const Core::MeshletHeader header =
        draw.m_geometry.Read<Core::MeshletHeader>(layout.m_headerOffset + meshletIndex * sizeof(Core::MeshletHeader));
    SetMeshOutputCounts(header.m_vertexCount, header.m_primitiveCount);

    if (lane < header.m_vertexCount)
    {
        const uint vertexIndex = draw.m_geometry.Read<uint>(layout.m_remapOffset + (header.m_vertexOffset + lane) * sizeof(uint));
        verts[lane] = TransformAttributes(draw, vertexIndex, GView.m_view.m_viewProjection, false, false);
    }

    if (lane < header.m_primitiveCount)
        tris[lane] = LoadPrimitive(draw, header.m_primitiveOffset + lane, layout.m_triangleOffset);
}
