#include <Shaders/Features/Tools/Blit/Blit.hlsli>

PixelAttributes main(const uint32_t index : SV_VertexID)
{
    const InlineMeshes::Quad::Vertex vertex = InlineMeshes::Quad::GetVertex(index);

    PixelAttributes attributes;
    attributes.m_position = vertex.m_position;
    attributes.m_texCoord = vertex.m_texCoord * GConstants.m_uvScale + GConstants.m_uvOffset;
    return attributes;
}
