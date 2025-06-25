#include <Shaders/Features/Tools/Blit/Blit.hlsli>

void main(const in PixelAttributes pixelAttributes, out float4 color : SV_Target0)
{
    color = GConstants.m_input.Sample(GConstants.m_sampler.Get(), pixelAttributes.m_texCoord);
}
