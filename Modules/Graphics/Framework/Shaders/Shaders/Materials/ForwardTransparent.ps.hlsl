#include <Shaders/Lighting/ForwardTransparency.h>
#include <Shaders/Lighting/GlobalLighting.hlsli>
#include <Shaders/Passes/MeshPass/Common.hlsli>
#include <Shaders/Shading/PBR.hlsli>
#include <Shaders/Shadows/SunShadow.hlsli>

struct MaterialParameters
{
    float4 BaseColor;
    float4 Properties;
};


// clang-format off
[earlydepthstencil]
void main(PixelAttributes input, out float4 accumulation : SV_Target0, out float4 revealage : SV_Target1,
          out float reactive : SV_Target2)
// clang-format on
{
    const MaterialInstanceTable table = GView.m_scene.Load().m_materials;
    const MaterialParameters parameters = table.ReadRow(input.m_materialRow).m_materialParameters.Get().Read<MaterialParameters>(0);
    const TransparencyLighting pass = GPass.m_passData.As<TransparencyLighting>().Load();
    ViewLighting lighting = pass.m_lighting.Load();
    lighting.m_sunShadow.m_feedback = pass.m_shadowFeedback;
    Shading::MaterialEvaluation material;
    material.m_surface.m_diffuseAlbedo = parameters.BaseColor.rgb * (1 - parameters.Properties.y);
    material.m_surface.m_f0 = lerp((float3)0.04f, parameters.BaseColor.rgb, parameters.Properties.y);
    material.m_surface.m_perceptualRoughness = max(0.04f, parameters.Properties.x);
    material.m_surface.m_worldSpaceNormal = normalize(input.m_normal);
    material.m_ambientOcclusion = 1;
    material.m_emissive = 0;
    const float sunVisibility =
        SunShadow::SampleVisibility(lighting.m_sunShadow, input.m_worldPos, material.m_surface.m_worldSpaceNormal);
    const float3 color = Shading::ComputeShading(material,
                                                 input.m_worldPos,
                                                 GView.m_view.m_cameraPosition.xyz,
                                                 LoadGlobalLighting(lighting.m_global),
                                                 lighting.m_local,
                                                 lighting.m_sunShadow,
                                                 input.m_pos.xy,
                                                 sunVisibility)
                             .rgb;
    const float alpha = saturate(parameters.BaseColor.a);
    // Bounded weights avoid half-float overflow while favoring closer layers in reversed Z.
    const float weight = clamp(pow(alpha + 0.01f, 3) * (0.1f + input.m_pos.z * input.m_pos.z * 100), 0.01f, 10.0f);
    accumulation = float4(color * alpha, alpha) * weight;
    revealage = alpha;
    reactive = min(alpha, 0.9f);
}
