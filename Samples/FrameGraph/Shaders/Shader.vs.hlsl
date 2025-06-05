struct PixelAttributes
{
    float4 m_pos : SV_Position;
    float3 m_color : COLOR;
};


struct VertexInput
{
    float3 m_pos : POSITION;
    float3 m_color : COLOR;
};


struct Constants
{
    float4x4 m_worldTransform;
};

[[vk::push_constant]] Constants GConstants;


PixelAttributes main_vs(const in VertexInput input)
{
    PixelAttributes output;
    output.m_pos = mul(float4(input.m_pos, 1.0f), GConstants.m_worldTransform);
    output.m_color = input.m_color;
    return output;
}
