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
    float3 g_Offset;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = float4(input.pos + g_Offset, 1.0f);
    output.texCoord = input.texCoord;
    return output;
}
