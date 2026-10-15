#include <Shaders/Deferred/EvaluateCommon.hlsli>
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

    const ViewData view = GConstants.m_view.Load(0);
    const uint2 id = GConstants.m_visibility.Load(uint3(pixel, 0));
    ReconstructedAttributes attributes;
    if (!ReconstructAttributes(view, id, float2(pixel) + 0.5f, float2(GConstants.m_resolution), attributes))
    {
        GConstants.m_g0.Store(pixel, 0xffff00ffu);
        return;
    }

    const MeshInstanceTable::Row instance = MeshInstanceTable::Create(view.m_instances).ReadRow(id.x - 1);
    const MeshGroupTable::Row group = MeshGroupTable::Create(view.m_groups).ReadRow(groupIndex);
    const MaterialInstanceTable::Row binding =
        MaterialInstanceTable::Create(view.m_materials).ReadRow(group.m_materialInstance.Get());
    const MaterialParameters material = binding.m_materialParameters.Get().Read<MaterialParameters>(0);

    const float2 scale = instance.m_instanceData.Get().Read<InstanceParameters>(0).UvScale.xy;
    const float2 uv = attributes.m_uv * scale;
    const float2 dx = attributes.m_uvDdx * scale;
    const float2 dy = attributes.m_uvDdy * scale;
    const float3 color = material.BaseColor.SampleGrad(GLinearWrapSampler, uv, dx, dy);
    const float3 orm = material.AORoughnessMetallic.SampleGrad(GLinearWrapSampler, uv, dx, dy);
    const float2 map = material.Normal.SampleGrad(GLinearWrapSampler, uv, dx, dy);
    const float3 emissive = material.Emissive.SampleGrad(GLinearWrapSampler, uv, dx, dy) * 1000.0f;

    PixelAttributes frame = (PixelAttributes)0;
    frame.m_normal = attributes.m_normal;
    frame.m_tangent = attributes.m_tangent;

    const float3 tangentNormal = Shading::UnpackNormal(map);
    const float3 normal = TangentNormalToWorld(frame, tangentNormal);

    GBuffer::Unpacked surface = (GBuffer::Unpacked)0;
    surface.m_color = color;
    surface.m_normal = normal;
    surface.m_shadingRoute = SurfaceShadingRoute(view, groupIndex);
    surface.m_roughness = orm.g;
    surface.m_metallic = orm.b;
    surface.m_ambientOcclusion = orm.r;
    surface.m_emissive = emissive;
    StoreGBuffer(pixel, surface);
}
