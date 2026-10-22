#include <Shaders/Passes/ImGui/ImGui.hlsli>

PixelAttributes main(const uint32_t vertexID : SV_VertexID)
{
    // Indices are 16-bit and tightly packed; vertices follow the ImDrawVert layout.
    const uint32_t indexPosition = GConstants.m_firstIndex + vertexID;
    const uint32_t indexWord = GConstants.m_indices.Load((indexPosition >> 1) * 4);
    const uint32_t index = (indexPosition & 1) ? indexWord >> 16 : indexWord & 0xffff;

    const uint32_t vertexAddress = (GConstants.m_vertexOffset + index) * kVertexByteSize;
    const float4 positionAndTexCoord = asfloat(GConstants.m_vertices.Load4(vertexAddress));
    const uint32_t color = GConstants.m_vertices.Load(vertexAddress + 16);

    PixelAttributes attributes;
    attributes.m_position = float4(positionAndTexCoord.xy * GConstants.m_scale + GConstants.m_translation, 0.0f, 1.0f);
    attributes.m_color = float4(color & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, color >> 24) / 255.0f;
    attributes.m_texCoord = positionAndTexCoord.zw;
    return attributes;
}
