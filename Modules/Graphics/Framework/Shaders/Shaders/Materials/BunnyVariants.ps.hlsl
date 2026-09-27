#include <Shaders/Passes/MeshPass/Common.hlsli>

void main(const in PixelAttributes input, out float4 output : SV_Target0)
{
    const float3 lightDirection = normalize(float3(0.4f, 0.7f, -0.2f));
    const float diffuse = max(dot(normalize(input.m_normal), lightDirection), 0.0f);

#if BUNNY_PALETTE == 1
    const float3 color = float3(0.25f, 0.7f, 0.9f);
#else
    const float3 color = float3(0.95f, 0.4f, 0.2f);
#endif

    output = float4(color * (0.15f + diffuse * 0.85f), 1.0f);
}
