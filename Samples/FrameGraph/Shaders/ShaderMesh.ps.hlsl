#include "Common.hlsli"

struct PixelAttributes
{
    float4 m_pos : SV_Position;
    float2 m_uv : TEXCOORD0;
    uint m_index : DEBUG_INDEX;
};


struct MeshLodInfo
{
    uint32_t m_vertexCount;
    uint32_t m_indexCount;
    uint32_t m_meshletCount;
    uint32_t m_primitiveCount;
};


struct Constants
{
    ByteAddressBufferDescriptor m_geometry;
    Texture2DDescriptor<float4> m_texture;
    SamplerDescriptor m_sampler;
    StructuredBufferDescriptor<float4x4> m_instanceData;
    MeshLodInfo m_lodInfo;
};

[[vk::push_constant]] Constants GConstants;

void main(const in PixelAttributes input, out float4 output : SV_Target0)
{
    const float3 texColor = GConstants.m_texture.Sample(GConstants.m_sampler.Get(), input.m_uv).rgb;
    const float3 idxColor = float3(float(input.m_index & 1), float(input.m_index & 3) / 4, float(input.m_index & 7) / 8);
    output = float4(lerp(texColor, idxColor, 0.2f), 1.0f);
}
