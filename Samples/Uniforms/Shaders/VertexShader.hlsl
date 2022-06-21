struct VSInput
{
    float3 pos : POSITION;
};

struct VSOutput
{
    float4 pos: SV_POSITION;
};

cbuffer Settings : register(b0, space1)
{
    float3 g_Offset;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = float4(input.pos + g_Offset, 1.0f);
    return output;
}
