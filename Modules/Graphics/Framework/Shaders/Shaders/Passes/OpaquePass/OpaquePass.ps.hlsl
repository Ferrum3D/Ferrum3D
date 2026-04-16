#include <Shaders/Passes/MeshPass/Common.hlsli>

void main(const in PixelAttributes input, out float4 output : SV_Target0)
{
    const float3 lightDirection = normalize(float3(0.4f, 0.7f, 0.2f));
    const float3 normal = normalize(input.m_normal);
    const float diffuse = max(dot(normal, lightDirection), 0.0f);
    const float3 color = GConstants.m_baseColor.rgb * (0.15f + diffuse * 0.85f);
    output = float4(color, GConstants.m_baseColor.a);
}
