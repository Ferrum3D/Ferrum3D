struct VSInput
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

cbuffer Settings : register(b2, space0)
{
float4x4 g_MVP;
}

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos      = mul(float4(input.pos, 1.0f), g_MVP);
    output.texCoord = input.texCoord;
    return output;
}
