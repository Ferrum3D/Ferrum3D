struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D g_Texture : register(t1, space0);
SamplerState g_Sampler : register(t0, space0);

float4 main(VSOutput input) : SV_TARGET
{
    return g_Texture.Sample(g_Sampler, input.texCoord);;
}
