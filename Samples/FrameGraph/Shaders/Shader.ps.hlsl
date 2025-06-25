#include "Common.hlsli"

struct PixelAttributes
{
    float4 m_pos : SV_Position;
    float2 m_uv : TEXCOORD0;
};


struct Constants
{
    Texture2DDescriptor<float4> m_texture;
    SamplerDescriptor m_sampler;
    StructuredBufferDescriptor<float4x4> m_instanceData;
    uint m_padding;
};

[[vk::push_constant]] Constants GConstants;

void main(const in PixelAttributes input, out float4 output : SV_Target0)
{
    output = GConstants.m_texture.Sample(GConstants.m_sampler.Get(), input.m_uv);
}
