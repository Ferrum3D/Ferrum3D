#include <Shaders/Materials/EvaluateMesh.hlsli>

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

    const MeshViewData view = LoadMeshView(GConstants.m_view);
    const uint2 id = GConstants.m_visibility.Load(uint3(pixel, 0));

    MeshEvaluationContext context;
    if (!LoadMeshEvaluation(view, id, pixel, GConstants.m_resolution, context, true, false))
    {
        GConstants.m_g0.Store(pixel, 0xffff00ffu);
        return;
    }

    const MaterialParameters material = context.m_materialParameters.Read<MaterialParameters>(0);
    const float2 scale = context.m_instanceParameters.Read<InstanceParameters>(0).UvScale.xy;
    const float2 uv = context.m_attributes.m_uv * scale;
    const float2 uvDdx = context.m_attributes.m_uvDdx * scale;
    const float2 uvDdy = context.m_attributes.m_uvDdy * scale;
    const float3 texel = material.Albedo.SampleGrad(GLinearWrapSampler, uv, uvDdx, uvDdy).rgb;
    const float3 color = texel * material.Tint.rgb;

    GBuffer::Unpacked surface = (GBuffer::Unpacked)0;
    surface.m_color = color;
    surface.m_normal = context.m_attributes.m_normal;
    surface.m_shadingRoute = context.m_shadingRoute;
    StoreGBuffer(pixel, surface);
}
