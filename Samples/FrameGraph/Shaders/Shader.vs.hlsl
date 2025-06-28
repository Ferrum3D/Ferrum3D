struct PixelAttributes
{
    float4 m_pos : SV_Position;
    float2 m_uv : TEXCOORD0;
};


struct VertexInput
{
    float3 m_pos : POSITION;
    float2 m_uv : TEXCOORD0;
};


struct Constants
{
    uint m_textureIndex;
    uint m_samplerIndex;
    uint m_instanceData;
    uint m_padding;
};

[[vk::push_constant]] Constants GConstants;

StructuredBuffer<float4x4> GBuffers[] : register(t3);


PixelAttributes main(const in VertexInput input)
{
    const float4x4 worldTransform = GBuffers[GConstants.m_instanceData][0];

    PixelAttributes output;
    output.m_pos = mul(float4(input.m_pos, 1.0f), worldTransform);
    output.m_uv = input.m_uv;
    return output;
}
