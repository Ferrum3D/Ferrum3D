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
    float4x4 m_worldTransform;
    uint m_textureIndex;
    uint m_samplerIndex;
    uint2 m_padding;
};

[[vk::push_constant]] Constants GConstants;


PixelAttributes main_vs(const in VertexInput input)
{
    PixelAttributes output;
    output.m_pos = mul(float4(input.m_pos, 1.0f), GConstants.m_worldTransform);
    output.m_uv = input.m_uv;
    return output;
}
