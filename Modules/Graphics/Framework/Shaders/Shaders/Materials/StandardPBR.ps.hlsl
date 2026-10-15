#include <Shaders/Passes/MeshPass/Common.hlsli>
#include <Shaders/Shading/PBR.hlsli>

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
    const float2 uv = input.m_uv * uvScale;
    const float3 baseColor = params.BaseColor.Sample(GLinearWrapSampler, uv);
    const float3 orm = params.AORoughnessMetallic.Sample(GLinearWrapSampler, uv);
    const float2 normal = params.Normal.Sample(GLinearWrapSampler, uv);
    const float3 emissive = params.Emissive.Sample(GLinearWrapSampler, uv);

    const float3 tangentNormal = Shading::UnpackNormal(normal);
    Shading::MaterialEvaluation evaluation;
    evaluation.m_surface.m_diffuseAlbedo = baseColor * (1.0f - orm.b);
    evaluation.m_surface.m_f0 = lerp((float3)0.04f, baseColor, orm.b);
    evaluation.m_surface.m_perceptualRoughness = orm.g;
    evaluation.m_surface.m_worldSpaceNormal = TangentNormalToWorld(input, tangentNormal);
    evaluation.m_ambientOcclusion = orm.r;
    evaluation.m_emissive = emissive * (1000.0f * Shading::kPreExposure); // Texture modulates 1000 cd/m^2.
    return evaluation;
}


void main(const in PixelAttributes input, out float4 output : SV_Target0)
{
    MaterialParameters material;
    InstanceParameters instance;
    LoadMaterialParameters(input.m_instanceIndex, material, instance);
    const Shading::MaterialEvaluation evaluation = EvaluateMaterial(input, material, instance);
    output = Shading::ComputeShading(evaluation, input.m_worldPos, GView.m_cameraPosition.xyz);
}
