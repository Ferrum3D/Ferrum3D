#include <Shaders/Materials/EvaluateMesh.hlsli>
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


FE_NUM_THREADS(EVALUATION_LANES, 1, 1)
void main(uint lane : SV_GroupThreadID, uint3 dispatch : SV_GroupID)
{
    uint2 pixel;
    uint groupIndex;
    if (!LoadEvaluationPixel(dispatch, lane, pixel, groupIndex))
        return;

    const MeshViewData view = LoadMeshView(GConstants.m_view);
    const uint2 id = GConstants.m_visibility.Load(uint3(pixel, 0));
    MeshEvaluationContext context;
    if (!LoadMeshEvaluation(view, id, pixel, GConstants.m_resolution, context))
    {
        GConstants.m_g0.Store(pixel, 0xffff00ffu);
        return;
    }

    const MaterialParameters material = context.m_materialParameters.Read<MaterialParameters>(0);

    const float2 scale = context.m_instanceParameters.Read<InstanceParameters>(0).UvScale.xy;
    const float2 uv = context.m_attributes.m_uv * scale;
    const float2 dx = context.m_attributes.m_uvDdx * scale;
    const float2 dy = context.m_attributes.m_uvDdy * scale;
    const float3 color = material.BaseColor.SampleGrad(GLinearWrapSampler, uv, dx, dy);
    const float3 orm = material.AORoughnessMetallic.SampleGrad(GLinearWrapSampler, uv, dx, dy);
    const float2 map = material.Normal.SampleGrad(GLinearWrapSampler, uv, dx, dy);
    const float3 emissive = material.Emissive.SampleGrad(GLinearWrapSampler, uv, dx, dy) * 1000.0f;

    PixelAttributes frame = (PixelAttributes)0;
    frame.m_normal = context.m_attributes.m_normal;
    frame.m_tangent = context.m_attributes.m_tangent;

    const float3 tangentNormal = Shading::UnpackNormal(map);
    const float3 normal = TangentNormalToWorld(frame, tangentNormal);

    GBuffer::Unpacked surface = (GBuffer::Unpacked)0;
    surface.m_color = color;
    surface.m_normal = normal;
    surface.m_shadingRoute = context.m_shadingRoute;
    surface.m_roughness = orm.g;
    surface.m_metallic = orm.b;
    surface.m_ambientOcclusion = orm.r;
    surface.m_emissive = emissive;
    StoreGBuffer(pixel, surface);
}
