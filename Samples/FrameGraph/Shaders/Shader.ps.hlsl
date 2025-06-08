struct PixelAttributes
{
    float4 m_pos : SV_Position;
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


SamplerState GSamplers[] : register(s0);
Texture2D GTextures[] : register(t1);


void main_ps(const in PixelAttributes input, out float4 output : SV_Target0)
{
    const Texture2D texture = GTextures[GConstants.m_textureIndex];
    const SamplerState sampler = GSamplers[GConstants.m_samplerIndex];
    output = texture.Sample(sampler, input.m_uv);
}
