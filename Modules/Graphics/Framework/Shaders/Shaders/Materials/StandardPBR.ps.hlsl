#include <Shaders/Core/Shading/PBR.hlsli>
#include <Shaders/Passes/MeshPass/Common.hlsli>

struct MaterialParameters
{
    Texture2DDescriptor<float3> BaseColor;
    Texture2DDescriptor<float3> AORoughnessMetallic;
    Texture2DDescriptor<float2> Normal;
    Texture2DDescriptor<float3> Emissive;
};


struct InstanceParameters
{
    float4 UvScale;
};


Shading::MaterialEvaluation EvaluateMaterial(const in PixelAttributes input, const in MaterialParameters params,
                                             const in InstanceParameters instance)
{
    const float2 uvScale = instance.UvScale.xy;
    const float3 baseColor = params.BaseColor.Sample(GLinearWrapSampler, input.m_uv * uvScale);
    const float3 orm = params.AORoughnessMetallic.Sample(GLinearWrapSampler, input.m_uv * uvScale);
    const float2 normal = params.Normal.Sample(GLinearWrapSampler, input.m_uv * uvScale);
    const float3 emissive = params.Emissive.Sample(GLinearWrapSampler, input.m_uv * uvScale);

    Shading::MaterialEvaluation evaluation;
    evaluation.m_surface.m_diffuseAlbedo = baseColor * (1.0f - orm.b);
    evaluation.m_surface.m_f0 = lerp((float3)0.04f, baseColor, orm.b);
    evaluation.m_surface.m_perceptualRoughness = orm.g;
    evaluation.m_surface.m_worldSpaceNormal = TangentNormalToWorld(input, Shading::UnpackNormal(normal));
    evaluation.m_ambientOcclusion = orm.r;
    evaluation.m_emissive = emissive * (1000.0f * Shading::kPreExposure); // Texture modulates 1000 cd/m^2.
    return evaluation;
}


void main(const in PixelAttributes input, out float4 output : SV_Target0)
{
    MaterialParameters material;
    InstanceParameters instance;
    LoadMaterialParameters(input.m_instanceIndex, material, instance);
    output = Shading::ComputeShading(EvaluateMaterial(input, material, instance), input.m_worldPos, GView.m_cameraPosition.xyz);
}
