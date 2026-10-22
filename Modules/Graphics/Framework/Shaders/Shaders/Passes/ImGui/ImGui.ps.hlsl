#include <Shaders/Passes/ImGui/ImGui.hlsli>

void main(const in PixelAttributes attributes, out float4 color : SV_Target0)
{
    // UI colors are authored for nonlinear presentation targets and blend without conversion.
    color = attributes.m_color * GConstants.m_texture.Sample(GConstants.m_sampler.Get(), attributes.m_texCoord);
}
