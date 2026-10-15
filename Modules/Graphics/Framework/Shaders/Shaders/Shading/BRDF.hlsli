#pragma once
#include <Shaders/Base/BaseTypes.hlsli>

namespace Shading
{
    float DistributionGGX(const float nDotH, const float alpha)
    {
        const float alphaSquared = alpha * alpha;
        const float denominator = nDotH * nDotH * (alphaSquared - 1.0f) + 1.0f;
        return alphaSquared / (Math::kPI * denominator * denominator);
    }


    float VisibilitySmithGGX(const float nDotV, const float nDotL, const float alpha)
    {
        const float alphaSquared = alpha * alpha;
        const float lambdaV = nDotL * sqrt(nDotV * nDotV * (1.0f - alphaSquared) + alphaSquared);
        const float lambdaL = nDotV * sqrt(nDotL * nDotL * (1.0f - alphaSquared) + alphaSquared);
        return 0.5f / max(lambdaV + lambdaL, 1.0e-6f);
    }


    float3 FresnelSchlick(const float vDotH, const float3 f0)
    {
        const float oneMinusCosine = 1.0f - vDotH;
        const float square = oneMinusCosine * oneMinusCosine;
        return f0 + (1.0f - f0) * square * square * oneMinusCosine;
    }


    float3 EvaluateBRDF(const float3 diffuseAlbedo, const float3 f0, const float perceptualRoughness, const float3 normal,
                        const float3 viewDirection, const float3 lightDirection)
    {
        const float nDotL = saturate(dot(normal, lightDirection));
        const float nDotV = saturate(dot(normal, viewDirection));
        if (nDotL <= 0.0f || nDotV <= 0.0f)
            return 0.0f;

        const float3 halfDirection = normalize(viewDirection + lightDirection);
        const float nDotH = saturate(dot(normal, halfDirection));
        const float vDotH = saturate(dot(viewDirection, halfDirection));
        const float roughness = max(saturate(perceptualRoughness), 0.045f);
        const float alpha = roughness * roughness;
        const float3 fresnel = FresnelSchlick(vDotH, f0);
        const float3 specular = DistributionGGX(nDotH, alpha) * VisibilitySmithGGX(nDotV, nDotL, alpha) * fresnel;
        const float3 diffuse = (1.0f - fresnel) * diffuseAlbedo / Math::kPI;
        return (diffuse + specular) * nDotL;
    }
} // namespace Shading
