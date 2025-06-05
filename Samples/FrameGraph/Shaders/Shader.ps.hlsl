struct PixelAttributes
{
    float4 m_pos : SV_Position;
    float3 m_color : COLOR;
};


float4 main_ps(const in PixelAttributes input)
    : SV_Target0
{
    return float4(input.m_color, 1.0f);
}
