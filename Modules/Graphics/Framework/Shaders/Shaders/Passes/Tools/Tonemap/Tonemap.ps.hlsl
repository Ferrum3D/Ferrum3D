#include <Shaders/Core/Shading/Color.hlsli>
#include <Shaders/Passes/Tools/Blit/Blit.hlsli>

void main(const in PixelAttributes input, out float4 color : SV_Target0)
{
    const float3 hdr = GConstants.m_input.Sample(GConstants.m_sampler.Get(), input.m_texCoord).rgb;
    color = float4(Shading::LinearToSRGB(Shading::TonemapACES(hdr)), 1.0f);
}
