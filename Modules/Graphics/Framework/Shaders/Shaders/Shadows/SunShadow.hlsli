#pragma once
#include <Shaders/Shadows/SunShadow.h>

namespace SunShadow
{
    //! A position projected into the virtual shadow map of one clip level.
    struct Projection
    {
        uint m_level;
        //! Texel coordinates in the virtual map of the level.
        float2 m_texel;
        float m_depth;
        bool m_isValid;
    };


    float2 ProjectTexel(const SunShadowData data, const uint level, const float3 worldPosition, out float depth)
    {
        const float4 clip = mul(float4(worldPosition, 1.0f), data.m_levels[level].m_viewProjection);
        depth = clip.z;
        return (clip.xy * float2(0.5f, -0.5f) + 0.5f) * kSunShadowVirtualSize;
    }


    // The finest level whose texels are no larger than the camera pixel at this position and that still covers it with a
    // margin for filtering. Marking and sampling must agree, so both call this function.
    Projection Project(const SunShadowData data, const float3 worldPosition)
    {
        const float distance = length(worldPosition - data.m_cameraPosition.xyz);
        const float pixelSize = data.m_cameraPosition.w * (data.m_isOrthographicCamera != 0 ? 1.0f : distance);
        const float levelBias = floor(log2(max(pixelSize / data.m_level0TexelSize, 1.0f)));

        Projection result = (Projection)0;
        for (uint level = (uint)levelBias; level < data.m_levelCount; ++level)
        {
            float depth;
            const float2 texel = ProjectTexel(data, level, worldPosition, depth);
            const bool isInside = all(texel >= 2.0f) && all(texel <= kSunShadowVirtualSize - 2.0f) && depth > 0.0f && depth < 1.0f;
            if (isInside)
            {
                result.m_level = level;
                result.m_texel = texel;
                result.m_depth = depth;
                result.m_isValid = true;
                return result;
            }
        }

        return result;
    }


    //! World page shown at a page of a level; it stays the same while the level scrolls by whole pages.
    int2 GetWorldPage(const int2 pageOrigin, const uint2 levelPage)
    {
        return int2(levelPage) + int2(pageOrigin.x, -pageOrigin.y);
    }


    //! Entries are addressed toroidally by world page, so scrolling keeps the entries of pages that remain in the level.
    uint GetEntryIndex(const uint level, const int2 worldPage)
    {
        const uint2 slot = uint2(worldPage & int2(kSunShadowLevelPagesPerAxis - 1, kSunShadowLevelPagesPerAxis - 1));
        return level * kSunShadowLevelPageCount + slot.y * kSunShadowLevelPagesPerAxis + slot.x;
    }


    uint GetPageIndex(const uint level, const int2 pageOrigin, const uint2 texel)
    {
        return GetEntryIndex(level, GetWorldPage(pageOrigin, texel / kSunShadowPageSize));
    }


    uint2 GetAtlasTexel(const uint pageEntry, const uint2 texel)
    {
        const uint physicalPage = pageEntry & kSunShadowPhysicalPageMask;
        const uint2 pageOrigin = uint2(physicalPage % kSunShadowAtlasPagesPerAxis, physicalPage / kSunShadowAtlasPagesPerAxis);
        return pageOrigin * kSunShadowPageSize + texel % kSunShadowPageSize;
    }


    //! Fraction of the sun visible from the position; 1 without shadow data or outside the clip levels.
    float SampleVisibility(const SunShadowBinding binding, const float3 worldPosition, const float3 normal)
    {
        if (!binding.m_data.IsValid())
            return 1.0f;

        const SunShadowData data = binding.m_data.Load(0);
        const Projection center = Project(data, worldPosition);
        if (!center.m_isValid)
            return 1.0f;

        // Offset along the normal by a few texels of the chosen level to avoid self-shadowing.
        const float texelSize = data.m_levels[center.m_level].m_texelSize;
        float receiverDepth;
        const float2 texel = ProjectTexel(data, center.m_level, worldPosition + normal * (2.0f * texelSize), receiverDepth);
        receiverDepth += texelSize * data.m_depthPerUnit;

        const int2 base = int2(floor(texel));
        const int2 pageOrigin = data.m_levels[center.m_level].m_pageOrigin;
        float visible = 0.0f;
        [unroll] for (int y = -1; y <= 1; ++y)
        {
            [unroll] for (int x = -1; x <= 1; ++x)
            {
                const uint2 tap = uint2(clamp(base + int2(x, y), 0, (int)kSunShadowVirtualSize - 1));
                // Marking requested every page the filter reads; others may hold content cached for other pixels.
                const uint entry = binding.m_pageTable.Load(GetPageIndex(center.m_level, pageOrigin, tap));
                const uint backedFlags = kSunShadowPageAllocated | kSunShadowPageRequested;
                if ((entry & backedFlags) != backedFlags)
                {
                    visible += 1.0f;
                    continue;
                }

                // Reverse Z: the occluder is closer to the sun when its depth is larger.
                const float occluderDepth = asfloat(binding.m_atlas.Load(uint3(GetAtlasTexel(entry, tap), 0)));
                visible += occluderDepth > receiverDepth ? 0.0f : 1.0f;
            }
        }

        return visible / 9.0f;
    }
} // namespace SunShadow
