#pragma once
#include <Shaders/Lighting/LightClusters.hlsli>
#include <Shaders/Shadows/SunShadow.hlsli>

namespace LocalShadow
{
    //! The camera whose pixels select the mips, taken from SunShadowData.
    struct Camera
    {
        //! xyz: position; w: world size of one of its pixels at unit distance.
        float4 m_positionFootprint;
        bool m_isOrthographic;
        bool m_isValid;
    };


    //! A position projected into one mip of a local shadow map.
    struct Projection
    {
        uint m_block;
        uint m_mip;
        //! Texel coordinates in the mip.
        float2 m_texel;
        float m_depth;
        bool m_isValid;
    };


    Camera CreateCamera(const SunShadowData data)
    {
        Camera camera;
        camera.m_positionFootprint = data.m_cameraPosition;
        camera.m_isOrthographic = data.m_isOrthographicCamera != 0;
        camera.m_isValid = true;
        return camera;
    }


    Camera LoadCamera(const SunShadowBinding binding)
    {
        Camera camera = (Camera)0;
        if (!binding.m_data.IsValid() || !binding.m_localLightMaps.IsValid())
            return camera;

        const SunShadowData data = binding.m_data.Load();
        camera.m_positionFootprint = data.m_cameraPosition;
        camera.m_isOrthographic = data.m_isOrthographicCamera != 0;
        camera.m_isValid = true;
        return camera;
    }


    uint LoadLightMaps(const SunShadowBinding binding, const uint lightIndex)
    {
        return binding.m_localLightMaps.IsValid() ? binding.m_localLightMaps.Load(lightIndex) : kLocalShadowNoMap;
    }


    //! The map that shows the position: the only map of a spot light, or the cube face of a point light.
    uint SelectMap(const uint lightMaps, const float3 lightPosition, const float3 worldPosition)
    {
        const uint firstMap = lightMaps & ~kLocalShadowPointFlag;
        if ((lightMaps & kLocalShadowPointFlag) == 0)
            return firstMap;

        // Faces are ordered +X, -X, +Y, -Y, +Z, -Z.
        const float3 delta = worldPosition - lightPosition;
        const float3 magnitude = abs(delta);
        uint axis = 2;
        if (magnitude.x >= magnitude.y && magnitude.x >= magnitude.z)
            axis = 0;
        else if (magnitude.y >= magnitude.z)
            axis = 1;

        return firstMap + axis * 2 + (delta[axis] < 0.0f ? 1 : 0);
    }


    //! World size of a texel of the mip at the position.
    float GetTexelSize(const LocalShadowMapData map, const uint mip, const float3 worldPosition)
    {
        const float distance = max(dot(worldPosition - map.m_positionTexelScale.xyz, map.m_forward), 0.0001f);
        return distance * map.m_positionTexelScale.w * float(1u << mip);
    }


    // The finest mip whose texels are no smaller than the camera pixel at the position. Marking and sampling must agree, so
    // both call this function.
    uint SelectMip(const Camera camera, const LocalShadowMapData map, const float3 worldPosition, const float levelBias = 0.0f)
    {
        const float distance = length(worldPosition - camera.m_positionFootprint.xyz);
        const float pixelSize = camera.m_positionFootprint.w * (camera.m_isOrthographic ? 1.0f : distance);
        const float texelSize = GetTexelSize(map, 0, worldPosition);
        return (uint)clamp(ceil(log2(max(pixelSize / texelSize, 1.0f))) + levelBias, 0.0f, float(kLocalShadowMipCount - 1));
    }


    Projection ProjectMip(const LocalShadowMapData map, const uint mip, const float3 worldPosition)
    {
        const float4 clip = mul(float4(worldPosition, 1.0f), map.m_viewProjection);
        const float size = float(kSunShadowVirtualSize >> mip);

        Projection result;
        result.m_block = map.m_firstBlock + mip;
        result.m_mip = mip;
        result.m_texel = (clip.xy / clip.w * float2(0.5f, -0.5f) + 0.5f) * size;
        result.m_depth = clip.z / clip.w;
        const bool isInside = all(result.m_texel >= 0.0f) && all(result.m_texel < size);
        result.m_isValid = clip.w > 0.0f && isInside && result.m_depth > 0.0f && result.m_depth < 1.0f;
        return result;
    }


    uint GetPageIndex(const Projection projection, const int2 texel)
    {
        const uint2 clamped = uint2(clamp(texel, 0, int(kSunShadowVirtualSize >> projection.m_mip) - 1));
        return SunShadow::GetPageIndex(projection.m_block, int2(0, 0), clamped);
    }


    bool IsBacked(const uint entry)
    {
        return SunShadow::IsBacked(entry);
    }


    void RequestProjection(RWStructuredBuffer<uint> requests, const Projection projection, const uint flags,
                           const bool isFeedback)
    {
        if (!projection.m_isValid)
            return;

        const int2 texel = int2(floor(projection.m_texel));
        const uint4 indices = uint4(GetPageIndex(projection, texel + int2(-3, -3)),
                                    GetPageIndex(projection, texel + int2(3, -3)),
                                    GetPageIndex(projection, texel + int2(-3, 3)),
                                    GetPageIndex(projection, texel + int2(3, 3)));
        SunShadow::RequestFootprint(requests, indices, flags, isFeedback);
    }


    void RequestLightPages(RWStructuredBuffer<uint> pageTable, const StructuredBufferDescriptor<LocalShadowMapData> maps,
                           const Camera camera, const uint lightMaps, const float3 worldPosition, const float levelBias,
                           const uint extraBias)
    {
        const LocalShadowMapData first = maps.Load(lightMaps & ~kLocalShadowPointFlag);
        const LocalShadowMapData map = maps.Load(SelectMap(lightMaps, first.m_positionTexelScale.xyz, worldPosition));
        const uint mip = min(SelectMip(camera, map, worldPosition, levelBias) + extraBias, kLocalShadowMipCount - 1);
        const uint flags = kSunShadowPageRequested | (extraBias != 0 ? kSunShadowPageCoarse : 0);
        RequestProjection(pageTable, ProjectMip(map, mip, worldPosition), flags, false);
    }


    //! Request the pages of every shadowed light in the light cluster of the position.
    void RequestPages(RWStructuredBuffer<uint> pageTable, const SunShadowData data, const LocalLightingBinding lighting,
                      const StructuredBufferDescriptor<LocalShadowMapData> maps,
                      const StructuredBufferDescriptor<uint32_t> lightMaps, const float3 worldPosition, const float2 pixel,
                      const uint extraBias = 0)
    {
        if (!lighting.m_clusters.IsValid() || !lightMaps.IsValid())
            return;

        const Camera camera = CreateCamera(data);
        const LightClusterData clusters = lighting.m_clusters.Load();
        const uint base = GetLightClusterBase(clusters, worldPosition, pixel);
        for (uint word = 0; word < clusters.m_wordCount; ++word)
        {
            const uint mask = lighting.m_masks.Load(base + word);
            // Scalarized like the shading loop, so the map loads stay uniform.
            uint merged = WaveActiveBitOr(mask);
            while (merged != 0)
            {
                const uint bit = firstbitlow(merged);
                merged &= ~(1u << bit);
                const uint lightMap = lightMaps.Load(word * 32 + bit);
                if (lightMap == kLocalShadowNoMap || (mask & (1u << bit)) == 0)
                    continue;

                RequestLightPages(pageTable, maps, camera, lightMap, worldPosition, data.m_levelBias, extraBias);
            }
        }
    }


    //! The map and mip that the lookups of a position read.
    struct MipSelection
    {
        LocalShadowMapData m_map;
        uint m_mip;
        bool m_isBacked;
    };


    //! The map of the light that shows the position, with the mip that SelectMip() selects or the next coarser one whose page
    //! under the position is backed. Pages are requested for this position, so the selection must not use one that is offset
    //! from it.
    MipSelection SelectBackedMip(const SunShadowBinding binding, const Camera camera, const uint lightMaps,
                                 const float3 worldPosition)
    {
        MipSelection result = (MipSelection)0;
        const LocalShadowMapData first = binding.m_localMaps.Load(lightMaps & ~kLocalShadowPointFlag);
        const uint mapIndex = SelectMap(lightMaps, first.m_positionTexelScale.xyz, worldPosition);
        result.m_map = binding.m_localMaps.Load(mapIndex);

        const uint selectedMip = SelectMip(camera, result.m_map, worldPosition, binding.m_data.Load().m_levelBias);
        if (binding.m_feedback.IsValid())
        {
            const Projection requested = ProjectMip(result.m_map, selectedMip, worldPosition);
            RequestProjection(binding.m_feedback.Get(), requested, kSunShadowPageRequested, true);
        }

        // Positions that requested no pages, e.g. transparent surfaces behind others, fall back to coarser mips.
        for (uint mip = selectedMip; mip < kLocalShadowMipCount && !result.m_isBacked; ++mip)
        {
            const Projection center = ProjectMip(result.m_map, mip, worldPosition);
            if (!center.m_isValid)
                return result;

            result.m_mip = mip;
            result.m_isBacked = IsBacked(binding.m_pageTable.Load(GetPageIndex(center, int2(floor(center.m_texel)))));
        }

        return result;
    }


    //! Fraction of the light visible from the receiver in the selected mip. The receiver may be offset towards the light
    //! from the position that selected the mip, which keeps its texels.
    float FilterMip(const SunShadowBinding binding, const MipSelection selection, const float3 receiverPosition,
                    const float3 normal)
    {
        // Offset along the normal and towards the light by a few texels of the chosen mip to avoid self-shadowing.
        const LocalShadowMapData map = selection.m_map;
        const float texelSize = GetTexelSize(map, selection.m_mip, receiverPosition);
        const float3 toLight = normalize(map.m_positionTexelScale.xyz - receiverPosition);
        const float3 offsetPosition = receiverPosition + (normal * 2.0f + toLight) * texelSize;
        const Projection receiver = ProjectMip(map, selection.m_mip, offsetPosition);
        const int2 base = int2(floor(receiver.m_texel));
        const int size = int(kSunShadowVirtualSize >> receiver.m_mip);

        // The taps usually fall into one page, which then needs a single page table load.
        const uint2 tapMin = uint2(clamp(base - 1, 0, size - 1));
        const uint2 tapMax = uint2(clamp(base + 1, 0, size - 1));
        const uint firstEntryIndex = SunShadow::GetPageIndex(receiver.m_block, int2(0, 0), tapMin);
        const bool isOnePage = firstEntryIndex == SunShadow::GetPageIndex(receiver.m_block, int2(0, 0), tapMax);
        const uint firstEntry = binding.m_pageTable.Load(firstEntryIndex);

        float visible = 0.0f;
        [unroll] for (int y = -1; y <= 1; ++y)
        {
            [unroll] for (int x = -1; x <= 1; ++x)
            {
                const uint2 tap = uint2(clamp(base + int2(x, y), 0, size - 1));
                uint entry = firstEntry;
                [branch] if (!isOnePage)
                    entry = binding.m_pageTable.Load(SunShadow::GetPageIndex(receiver.m_block, int2(0, 0), tap));

                if (!IsBacked(entry))
                {
                    visible += 1.0f;
                    continue;
                }

                // Reverse Z: the occluder is closer to the light when its depth is larger.
                const float occluderDepth = SunShadow::LoadDepth(binding, entry, tap);
                visible += occluderDepth > receiver.m_depth ? 0.0f : 1.0f;
            }
        }

        return visible / 9.0f;
    }


    //! Fraction of a local light visible from the position; 1 for lights without a shadow map.
    float SampleVisibility(const SunShadowBinding binding, const Camera camera, const uint lightMaps, const float3 worldPosition,
                           const float3 normal)
    {
        if (!camera.m_isValid || lightMaps == kLocalShadowNoMap)
            return 1.0f;

        const MipSelection selection = SelectBackedMip(binding, camera, lightMaps, worldPosition);
        if (!selection.m_isBacked)
            return 1.0f;

        return FilterMip(binding, selection, worldPosition, normal);
    }


    //! SampleVisibility for a surface in the camera depth, with the short ray of SunShadow::SampleVisibility towards the
    //! light. The ray passes through the light, so the lookup at its end reads the same texels as at the surface, which
    //! selects the map and the mip.
    float SampleVisibility(const SunShadowBinding binding, const Camera camera, const uint lightMaps,
                           const ScreenTrace::Context trace, const float3 worldPosition, const float3 normal,
                           const float3 directionToLight)
    {
        if (!camera.m_isValid || lightMaps == kLocalShadowNoMap)
            return 1.0f;

        const MipSelection selection = SelectBackedMip(binding, camera, lightMaps, worldPosition);
        if (!selection.m_isBacked)
            return 1.0f;

        const float traceTexels = binding.m_data.Load().m_screenTraceTexels;
        const bool isEnabled = trace.m_isValid && traceTexels > 0.0f && !camera.m_isOrthographic;
        if (!isEnabled || dot(normal, directionToLight) <= 0.0f)
            return FilterMip(binding, selection, worldPosition, normal);

        // The ray stays well short of the light.
        const LocalShadowMapData map = selection.m_map;
        const float lightDistance = length(map.m_positionTexelScale.xyz - worldPosition);
        const float texelSize = GetTexelSize(map, selection.m_mip, worldPosition);
        const float rayLength = min(texelSize * traceTexels, lightDistance * 0.5f);
        const float pixelSize = camera.m_positionFootprint.w * length(worldPosition - camera.m_positionFootprint.xyz);
        if (ScreenTrace::IsOccluded(trace, worldPosition + normal * pixelSize, directionToLight, rayLength, pixelSize, 4))
            return 0.0f;

        return FilterMip(binding, selection, worldPosition + directionToLight * rayLength, normal);
    }
} // namespace LocalShadow
