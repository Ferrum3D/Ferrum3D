#pragma once
#include <Shaders/Lighting/Atmosphere.hlsli>
#include <Shaders/Lighting/LocalLighting.hlsli>
#include <Shaders/Shading/Exposure.hlsli>

namespace Shading
{
    struct MaterialEvaluation
    {
        Surface m_surface;

        float3 m_emissive;
        float m_ambientOcclusion;
    };


    float3 UnpackNormal(const float2 packedNormal)
    {
        const float2 xy = packedNormal * 2.0f - 1.0f;
        const float z = sqrt(saturate(1.0f - dot(xy, xy)));
        return normalize(float3(xy, z));
    }


    //! sunVisibility scales the directional light, e.g. by its shadow.
    float4 ComputeShading(const MaterialEvaluation material, const float3 worldPosition, const float3 cameraPosition,
                          const GlobalLightingData lighting, const LocalLightingBinding localLighting, const float2 pixel,
                          const float sunVisibility = 1.0f)
    {
        const Surface surface = material.m_surface;
        const float3 viewDirection = normalize(cameraPosition - worldPosition);
        const float3 localLuminance = ComputeLocalLighting(localLighting, surface, worldPosition, cameraPosition, pixel);
        if (lighting.m_directionIlluminance.w <= 0)
            return float4(localLuminance + material.m_emissive, 1);

        const float3 lightDirection = lighting.m_directionIlluminance.xyz;
        const float3 preExposedIlluminance =
            Atmosphere::DirectionalIlluminance(lighting, worldPosition) * (kPreExposure * sunVisibility);
        const float3 brdf = EvaluateBRDF(surface.m_diffuseAlbedo,
                                         surface.m_f0,
                                         surface.m_perceptualRoughness,
                                         surface.m_worldSpaceNormal,
                                         viewDirection,
                                         lightDirection);
        const float3 reflectedLuminance = brdf * preExposedIlluminance;
        // AO belongs to indirect illumination, which is currently absent.
        return float4(reflectedLuminance + localLuminance + material.m_emissive, 1.0f);
    }
} // namespace Shading
