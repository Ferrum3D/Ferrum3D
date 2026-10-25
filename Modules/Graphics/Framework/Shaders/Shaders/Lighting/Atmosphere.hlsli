#pragma once
#include <Shaders/Lighting/GlobalLighting.hlsli>
#include <Shaders/Shading/Exposure.hlsli>

namespace Atmosphere
{
    static const float kPi = 3.14159265359f;

    float3 DirectionalIlluminance(GlobalLightingData lighting, float3 worldPosition)
    {
        return lighting.m_sunIlluminance.rgb * lighting.m_colorSunRadius.rgb * lighting.m_directionIlluminance.w;
    }


    float3 SkyLuminance(GlobalLightingData lighting, float3 worldPosition, float3 direction)
    {
        if (lighting.m_directionIlluminance.w <= 0 || lighting.m_directionIlluminance.y < 0)
            return 0;

        const float3 scale = lighting.m_colorSunRadius.rgb * lighting.m_directionIlluminance.w;
        if (direction.y < 0)
            return lighting.m_groundRadiance.rgb * scale;

        const float cosine = clamp(dot(direction, lighting.m_directionIlluminance.xyz), -1, 1);
        const float gamma = acos(cosine);
        const float zenith = saturate(direction.y);
        const float3 a = lighting.m_skyCoefficients[0].rgb;
        const float3 b = lighting.m_skyCoefficients[1].rgb;
        const float3 c = lighting.m_skyCoefficients[2].rgb;
        const float3 d = lighting.m_skyCoefficients[3].rgb;
        const float3 e = lighting.m_skyCoefficients[4].rgb;
        const float3 f = lighting.m_skyCoefficients[5].rgb;
        const float3 g = lighting.m_skyCoefficients[6].rgb;
        const float3 h = lighting.m_skyCoefficients[7].rgb;
        const float3 i = lighting.m_skyCoefficients[8].rgb;
        const float3 mie = (1 + cosine * cosine) / pow(max(1 + i * i - 2 * i * cosine, 0.0001f), 1.5f);
        float3 luminance = max((1 + a * exp(b / (zenith + 0.01f)))
            * (c + d * exp(e * gamma) + f * cosine * cosine + g * mie + h * sqrt(zenith)), 0)
            * lighting.m_skyRadiance.rgb;
        if (cosine > cos(lighting.m_colorSunRadius.w))
        {
            const float sineRadius = sin(lighting.m_colorSunRadius.w);
            luminance += lighting.m_sunIlluminance.rgb / (kPi * sineRadius * sineRadius);
        }

        return luminance * scale;
    }
} // namespace Atmosphere
