#include <Shaders/Passes/Visibility/Deferred.h>
#include <Shaders/Passes/Visibility/SurfaceCodec.hlsli>
#include <Shaders/Core/Shading/PBR.hlsli>
[[vk::push_constant]] DeferredConstants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 pixel : SV_DispatchThreadID)
{
    if (any(pixel.xy >= GConstants.m_resolution))
        return;

    const uint color = GConstants.m_g0.Load(uint3(pixel.xy, 0));
    if ((color >> 24) == 0 || (color >> 24) == 255)
        return;

    const float3 baseColor = DecodeSurfaceColor(color);
    const uint properties = GConstants.m_g2.Load(uint3(pixel.xy, 0));
    const float metallic = float((properties >> 8) & 255) / 255.0f;
    Shading::MaterialEvaluation material;
    material.m_surface.m_diffuseAlbedo = baseColor * (1 - metallic);
    material.m_surface.m_f0 = lerp((float3)0.04f, baseColor, metallic);
    material.m_surface.m_perceptualRoughness = float(properties & 255) / 255.0f;
    material.m_surface.m_worldSpaceNormal = DecodeSurfaceNormal(GConstants.m_g1.Load(uint3(pixel.xy, 0)));
    material.m_ambientOcclusion = float((properties >> 16) & 255) / 255.0f;
    material.m_emissive = (properties >> 24) & kExtendedPayloadValid
        ? DecodeSurfaceEmissive(GConstants.m_g3.Load(uint3(pixel.xy, 0))) * Shading::kPreExposure : 0;
    const float2 ndc = (float2(pixel.xy) + 0.5f) / float2(GConstants.m_resolution) * float2(2, -2) + float2(-1, 1);
    const float depth = GConstants.m_depth.Load(uint3(pixel.xy, 0));
    const float4 world = mul(float4(ndc, depth, 1), GConstants.m_inverseViewProjection);
    GConstants.m_output.Store(pixel.xy, Shading::ComputeShading(material, world.xyz / world.w, GConstants.m_view.Load(0).m_cameraPosition.xyz).rgb);
}
