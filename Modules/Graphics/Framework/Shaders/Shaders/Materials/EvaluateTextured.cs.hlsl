#include <Shaders/Deferred/EvaluateCommon.hlsli>

struct MaterialParameters
{
    Texture2DDescriptor<float4> Albedo;
    float4 Tint;
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
    if (!ReconstructAttributes(view, id, float2(pixel) + 0.5f, float2(GConstants.m_resolution), attributes, true, false))
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
    const float2 uvDdx = attributes.m_uvDdx * scale;
    const float2 uvDdy = attributes.m_uvDdy * scale;
    const float3 texel = material.Albedo.SampleGrad(GLinearWrapSampler, uv, uvDdx, uvDdy).rgb;
    const float3 color = texel * material.Tint.rgb;

    GBuffer::Unpacked surface = (GBuffer::Unpacked)0;
    surface.m_color = color;
    surface.m_normal = attributes.m_normal;
    surface.m_shadingRoute = SurfaceShadingRoute(view, groupIndex);
    StoreGBuffer(pixel, surface);
}
