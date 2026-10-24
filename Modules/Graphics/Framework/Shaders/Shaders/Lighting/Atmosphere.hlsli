#pragma once
#include <Shaders/Lighting/GlobalLighting.hlsli>
#include <Shaders/Shading/Exposure.hlsli>

namespace Atmosphere
{
    static const float kPi = 3.14159265359f;

    // Planet-relative distances and extinction coefficients use kilometers.
    float2 IntersectSphere(float3 origin, float3 direction, float radius)
    {
        const float b = dot(origin, direction);
        const float c = dot(origin, origin) - radius * radius;
        const float discriminant = b * b - c;
        if (discriminant < 0)
            return float2(1, -1);

        const float root = sqrt(discriminant);
        return float2(-b - root, -b + root);
    }


    float3 Density(GlobalLightingData lighting, float3 position)
    {
        const float height = max(length(position) - lighting.m_planetAtmosphereOzone.x, 0);
        const float rayleigh = exp(-height / lighting.m_rayleighScaleHeight.w);
        const float mie = exp(-height / lighting.m_mieScatteringScaleHeight.w);
        const float ozone = saturate(1 - abs(height - lighting.m_ozoneAbsorptionCenter.w) / lighting.m_planetAtmosphereOzone.z);
        return float3(rayleigh, mie, ozone);
    }


    float3 Extinction(GlobalLightingData lighting, float3 density)
    {
        return density.x * lighting.m_rayleighScaleHeight.rgb + density.y * lighting.m_mieExtinctionAnisotropy.rgb
            + density.z * lighting.m_ozoneAbsorptionCenter.rgb;
    }


    float3 SunTransmittance(GlobalLightingData lighting, float3 position)
    {
        const float3 direction = lighting.m_directionIlluminance.xyz;
        const float radius = lighting.m_planetAtmosphereOzone.x;
        const float2 ground = IntersectSphere(position, direction, radius);
        if (ground.x > 0 && ground.y >= ground.x)
            return 0;

        const float2 atmosphere = IntersectSphere(position, direction, radius + lighting.m_planetAtmosphereOzone.y);
        if (atmosphere.y <= 0)
            return 1;

        float3 opticalDepth = 0;
        [unroll] for (uint step = 0; step < 8; ++step)
        {
            const float nearT = float(step) / 8;
            const float farT = float(step + 1) / 8;
            const float start = nearT * nearT * atmosphere.y;
            const float end = farT * farT * atmosphere.y;
            opticalDepth += Density(lighting, position + direction * (0.5f * (start + end))) * (end - start);
        }

        return exp(-Extinction(lighting, opticalDepth));
    }


    float3 PlanetPosition(GlobalLightingData lighting, float3 worldPosition)
    {
        // World units are meters. The planet touches the origin, with its center on -Y.
        const float radius = lighting.m_planetAtmosphereOzone.x;
        return float3(worldPosition.x * 0.001f, radius + max(worldPosition.y * 0.001f, 0.001f), worldPosition.z * 0.001f);
    }


    float3 DirectionalIlluminance(GlobalLightingData lighting, float3 worldPosition)
    {
        if (lighting.m_directionIlluminance.w <= 0)
            return 0;

        return lighting.m_colorSunRadius.rgb * lighting.m_directionIlluminance.w
            * SunTransmittance(lighting, PlanetPosition(lighting, worldPosition));
    }


    float3 SkyLuminance(GlobalLightingData lighting, float3 worldPosition, float3 direction)
    {
        if (lighting.m_directionIlluminance.w <= 0)
            return 0;

        const float radius = lighting.m_planetAtmosphereOzone.x;
        float3 origin = PlanetPosition(lighting, worldPosition);
        const float cosine = dot(direction, lighting.m_directionIlluminance.xyz);
        const float2 atmosphere = IntersectSphere(origin, direction, radius + lighting.m_planetAtmosphereOzone.y);
        if (atmosphere.y < 0 || atmosphere.x > atmosphere.y)
        {
            const float sineRadius = sin(lighting.m_colorSunRadius.w);
            const float disc = cosine > cos(lighting.m_colorSunRadius.w) ? 1 / (kPi * sineRadius * sineRadius) : 0;
            return disc * lighting.m_colorSunRadius.rgb * lighting.m_directionIlluminance.w;
        }

        const float start = max(atmosphere.x, 0);
        const float2 ground = IntersectSphere(origin, direction, radius);
        const bool hitsGround = ground.x > start && ground.y >= ground.x;
        const float end = hitsGround ? min(ground.x, atmosphere.y) : atmosphere.y;
        const float distance = max(end - start, 0);
        const float rayleighPhase = 3 * (1 + cosine * cosine) / (16 * kPi);
        const float g = lighting.m_mieExtinctionAnisotropy.w;
        const float miePhase = 3 * (1 - g * g) * (1 + cosine * cosine)
            / (8 * kPi * (2 + g * g) * pow(max(1 + g * g - 2 * g * cosine, 0.0001f), 1.5f));
        float3 opticalDepth = 0;
        float3 luminance = 0;
        [loop] for (uint step = 0; step < 24; ++step)
        {
            const float nearT = float(step) / 24;
            const float farT = float(step + 1) / 24;
            const float nearDistance = nearT * nearT * distance;
            const float farDistance = farT * farT * distance;
            const float stepLength = farDistance - nearDistance;
            const float3 position = origin + direction * (start + 0.5f * (nearDistance + farDistance));
            const float3 density = Density(lighting, position);
            const float3 transmittance = exp(-Extinction(lighting, opticalDepth + density * (0.5f * stepLength)));
            const float3 scattering = density.x * lighting.m_rayleighScaleHeight.rgb * rayleighPhase
                + density.y * lighting.m_mieScatteringScaleHeight.rgb * miePhase;
            luminance += transmittance * SunTransmittance(lighting, position) * scattering * stepLength;
            opticalDepth += density * stepLength;
        }

        const float3 viewTransmittance = exp(-Extinction(lighting, opticalDepth));
        if (hitsGround)
        {
            const float3 position = origin + direction * end;
            const float nDotL = saturate(dot(normalize(position), lighting.m_directionIlluminance.xyz));
            luminance += viewTransmittance * SunTransmittance(lighting, position * (1 + 0.001f / radius))
                * lighting.m_groundAlbedo.rgb * (nDotL / kPi);
        }
        else if (cosine > cos(lighting.m_colorSunRadius.w))
        {
            // A uniform disc integrates to the authored perpendicular illuminance.
            const float sineRadius = sin(lighting.m_colorSunRadius.w);
            luminance += viewTransmittance / (kPi * sineRadius * sineRadius);
        }

        return luminance * lighting.m_colorSunRadius.rgb * lighting.m_directionIlluminance.w;
    }
} // namespace Atmosphere
