#include <Shaders/Deferred/GBuffer.hlsli>
#include <Shaders/Shading/PBR.hlsli>
#include <Shaders/Tests/DeferredProbe.h>
[[vk::push_constant]] Constants GConstants;

FE_NUM_THREADS(8, 8, 1)
void main(uint3 dispatch : SV_DispatchThreadID)
{
    const uint2 pixel = dispatch.xy;
    if (any(pixel >= GConstants.m_resolution))
        return;

    const uint index = pixel.x + pixel.y * GConstants.m_resolution.x;
    const DeferredViewData view = GConstants.m_view.Load(0);
    const float3 sourceColor = float3(0.4f, 0.6f, 0.8f);
    const float3 encodedColor = round(Shading::LinearToSRGB(sourceColor) * 255) / 255.0f;
    const float3 baseColor = Shading::SRGBToLinear(encodedColor);
    float3 expected = view.m_backgroundColor.rgb;
    if (GConstants.m_empty == 0 && index % 7 != 0)
    {
        if (index % 11 == 0 || index % 13 == 0)
            expected = float3(1, 0, 1);
        else if (index % 5 != 0)
            expected = baseColor * (0.15f + 0.85f * dot(float3(0, 0, -1), normalize(float3(0.4f, 0.7f, -0.2f))));
        else
        {
            Shading::MaterialEvaluation material;
            const float metallic = 64.0f / 255.0f;
            material.m_surface.m_diffuseAlbedo = baseColor * (1 - metallic);
            material.m_surface.m_f0 = lerp((float3)0.04f, baseColor, metallic);
            material.m_surface.m_perceptualRoughness = 140.0f / 255.0f;
            material.m_surface.m_worldSpaceNormal = float3(0, 0, -1);
            material.m_ambientOcclusion = 34.0f / 255.0f;
            material.m_emissive = (index & 1) != 0 ? float3(32, 64, 128) * Shading::kPreExposure : 0;
            const float2 ndc = float2(2 * (float(pixel.x) + 0.5f - 3) / 251 - 1, 1 - 2 * (float(pixel.y) + 0.5f - 5) / 119);
            const float depth = 0.2f + 0.6f * float(pixel.x) / float(GConstants.m_resolution.x);
#if CAMERA_PROBE
            // Independently undo the perspective projection, then apply the authored camera pose.
            const float z = (1000.0f * 0.1f) / (depth * (1000.0f - 0.1f) + 0.1f);
            const float3 cameraSpace = float3(ndc * float2(1.7f, 1) * tan(0.5f) * z, z);
            const float3 rotatedX = float3(cameraSpace.x,
                                           cameraSpace.y * cos(0.7f) - cameraSpace.z * sin(0.7f),
                                           cameraSpace.y * sin(0.7f) + cameraSpace.z * cos(0.7f));
            const float3 worldPosition = float3(rotatedX.x * cos(-1.1f) + rotatedX.z * sin(-1.1f),
                                                rotatedX.y,
                                                -rotatedX.x * sin(-1.1f) + rotatedX.z * cos(-1.1f))
                + float3(3, -2, 5);
#else
            const float4 world = mul(float4(ndc, depth, 1), view.m_inverseViewProjection);
            const float3 worldPosition = world.xyz / world.w;
#endif
            expected = Shading::ComputeShading(material, worldPosition, view.m_cameraPosition.xyz).rgb;
        }
    }

    const float3 actual = GConstants.m_color.Load(uint3(pixel, 0));
    const float3 relative = abs(actual - expected) / max(abs(expected), (float3)0.001f);
    const float error = max(relative.r, max(relative.g, relative.b));
    InterlockedMax(GConstants.m_stats.Get()[1], asuint(error));
#if WIDE_HDR || CAMERA_PROBE
    const float tolerance = 0.0012f;
#else
    // The packed blue channel has five mantissa bits; storage conversion can truncate.
    const float tolerance = 0.032f;
#endif
    if (!all(isfinite(actual)) || error > tolerance)
        InterlockedAdd(GConstants.m_stats.Get()[0], 1);

    InterlockedAdd(GConstants.m_stats.Get()[2], 1);
}
