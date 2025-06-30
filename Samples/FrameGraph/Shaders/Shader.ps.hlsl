#include "Common.hlsli"

struct PixelAttributes
{
    float4 m_pos : SV_Position;
    float2 m_uv : TEXCOORD0;
};


struct Constants
{
    ImageSRVDescriptor m_texture;
    SamplerDescriptor m_sampler;
    BufferSRVDescriptor m_instanceData;
    uint m_padding;
};

[[vk::push_constant]] Constants GConstants;


void main(const in PixelAttributes input, out float4 output : SV_Target0)
{
    const Texture2D texture = GConstants.m_texture.Get2D<float4>();
    const SamplerState sampler = GConstants.m_sampler.Get();
    output = texture.Sample(sampler, input.m_uv);
}
