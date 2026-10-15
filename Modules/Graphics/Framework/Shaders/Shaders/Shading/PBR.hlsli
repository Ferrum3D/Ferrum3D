#pragma once
#include <Shaders/Shading/BRDF.hlsli>
#include <Shaders/Shading/Exposure.hlsli>

namespace Shading
{
    struct Surface
    {
        float3 m_diffuseAlbedo;
        float3 m_f0;
        float m_perceptualRoughness;
        float3 m_worldSpaceNormal;
    };


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


    float4 ComputeShading(const MaterialEvaluation material, const float3 worldPosition, const float3 cameraPosition)
    {
        const Surface surface = material.m_surface;
        const float3 viewDirection = normalize(cameraPosition - worldPosition);
        const float3 lightDirection = normalize(float3(0.4f, 0.7f, -0.6f));
        const float illuminanceLux = 10000.0f;
        const float3 preExposedIlluminance = illuminanceLux * kPreExposure;
        const float3 brdf = EvaluateBRDF(surface.m_diffuseAlbedo,
                                         surface.m_f0,
                                         surface.m_perceptualRoughness,
                                         surface.m_worldSpaceNormal,
                                         viewDirection,
                                         lightDirection);
        const float3 reflectedLuminance = brdf * preExposedIlluminance;
        // AO belongs to indirect illumination, which is currently absent.
        return float4(reflectedLuminance + material.m_emissive, 1.0f);
    }
} // namespace Shading
