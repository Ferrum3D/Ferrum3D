#include "Common.hlsli"

struct PixelAttributes
{
    float4 m_pos : SV_Position;
    float2 m_uv : TEXCOORD0;
    uint m_index : DEBUG_INDEX;
};


struct VertexInput
{
    float3 m_pos : POSITION;
    float2 m_uv : TEXCOORD0;
};


struct Constants
{
    Texture2DDescriptor<float4> m_texture;
    SamplerDescriptor m_sampler;
    StructuredBufferDescriptor<float4x4> m_instanceData;
    uint m_padding;
};

[[vk::push_constant]] Constants GConstants;


PixelAttributes main(const in VertexInput input)
{
    const float4x4 worldTransform = GConstants.m_instanceData.Load(0);

    PixelAttributes output;
    output.m_pos = mul(float4(input.m_pos, 1.0f), worldTransform);
    output.m_uv = input.m_uv;
    output.m_index = 0;
    return output;
}
