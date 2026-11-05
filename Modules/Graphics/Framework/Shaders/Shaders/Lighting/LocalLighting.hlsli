#pragma once
#include <Shaders/Lighting/LightClusters.hlsli>
#include <Shaders/Lighting/LocalLightTable.hlsli>
#include <Shaders/Shading/BRDF.hlsli>
#include <Shaders/Shading/Exposure.hlsli>
#include <Shaders/Shading/Surface.hlsli>
#include <Shaders/Shadows/LocalShadow.hlsli>

namespace Shading
{
    float3 ComputeLocalLighting(LocalLightingBinding binding, SunShadowBinding shadows, Surface surface, float3 position,
                                float3 camera, float2 pixel)
    {
        if (!binding.m_clusters.IsValid())
            return 0;

        const LightClusterData data = binding.m_clusters.Load();
        const uint base = GetLightClusterBase(data, position, pixel);
        const LocalShadow::Camera shadowCamera = LocalShadow::LoadCamera(shadows);
        float3 result = 0;
        const float3 viewDirection = normalize(camera - position);
        for (uint word = 0; word < data.m_wordCount; ++word)
        {
            uint mask = binding.m_masks.Load(base + word);
            // Entity-level scalarization keeps light loads coherent even across cluster boundaries.
            uint merged = WaveActiveBitOr(mask);
            while (merged != 0)
            {
                const uint bit = firstbitlow(merged);
                merged &= ~(1u << bit);
                const DecodedLocalLight light = LoadLocalLight(binding, word * 32 + bit);
                const uint lightMaps = LocalShadow::LoadLightMaps(shadows, word * 32 + bit);
                if ((mask & (1u << bit)) == 0)
                    continue;

                const float3 delta = light.m_positionRange.xyz - position;
                const float distanceSquared = dot(delta, delta);
                const float3 direction = delta * rsqrt(max(distanceSquared, 0.000001f));
                const float rangeSquared = light.m_positionRange.w * light.m_positionRange.w;
                const float window = saturate(1 - (distanceSquared * distanceSquared) / (rangeSquared * rangeSquared));
                float attenuation = window * window / max(distanceSquared, 0.0001f);
                if (light.m_directionOuterCos.w > -1)
                {
                    const float cosine = dot(-direction, light.m_directionOuterCos.xyz);
                    const float cone = saturate((cosine - light.m_directionOuterCos.w)
                                                / max(light.m_cone.x - light.m_directionOuterCos.w, 0.000001f));
                    attenuation *= cone * cone * (3 - 2 * cone);
                }

                if (attenuation <= 0)
                    continue;

                attenuation *=
                    LocalShadow::SampleVisibility(shadows, shadowCamera, lightMaps, position, surface.m_worldSpaceNormal);

                result += EvaluateBRDF(surface.m_diffuseAlbedo,
                                       surface.m_f0,
                                       surface.m_perceptualRoughness,
                                       surface.m_worldSpaceNormal,
                                       viewDirection,
                                       direction)
                    * light.m_colorCandela.rgb * (light.m_colorCandela.w * attenuation * kPreExposure);
            }
        }

        return result;
    }
} // namespace Shading
