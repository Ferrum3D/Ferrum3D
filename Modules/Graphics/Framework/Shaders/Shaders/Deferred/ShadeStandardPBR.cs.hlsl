#include <Shaders/Deferred/DeferredCommon.hlsli>
#include <Shaders/Shading/PBR.hlsli>

FE_NUM_THREADS(EVALUATION_LANES, 1, 1)
void main(uint3 dispatch : SV_GroupID, uint lane : SV_GroupIndex)
{
    uint2 pixel;
    if (!LoadShadingPixel(dispatch, lane, pixel))
        return;

    const GBuffer::Unpacked surface = LoadGBuffer(pixel);
    if (surface.m_shadingRoute == kBackgroundShadingRoute || surface.m_shadingRoute == kUnsupportedShadingRoute)
        return;

    Shading::MaterialEvaluation material;
    material.m_surface.m_diffuseAlbedo = surface.m_color * (1 - surface.m_metallic);
    material.m_surface.m_f0 = lerp((float3)0.04f, surface.m_color, surface.m_metallic);
    material.m_surface.m_perceptualRoughness = surface.m_roughness;
    material.m_surface.m_worldSpaceNormal = surface.m_normal;
    material.m_ambientOcclusion = surface.m_ambientOcclusion;
    material.m_emissive = surface.m_emissive * Shading::kPreExposure;

    const DeferredViewData view = GConstants.m_view.Load(0);
    const float2 viewportPosition = (float2(pixel) + 0.5f - view.m_viewport.xy) / view.m_viewport.zw;
    const float2 ndc = viewportPosition * float2(2, -2) + float2(-1, 1);
    const float depth = GConstants.m_depth.Load(uint3(pixel, 0));
    const float4 homogeneousWorld = mul(float4(ndc, depth, 1), view.m_inverseViewProjection);
    const float3 worldPosition = homogeneousWorld.xyz / homogeneousWorld.w;
    const float3 color = Shading::ComputeShading(material, worldPosition, view.m_cameraPosition.xyz).rgb;
    GConstants.m_output.Store(pixel, color);
}
