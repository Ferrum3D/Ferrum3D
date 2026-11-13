#pragma once
#include <Shaders/Shadows/ScreenTrace.hlsli>
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


    float2 ProjectTexel(const SunShadowLevel level, const float3 worldPosition, out float depth)
    {
        const float4 clip = mul(float4(worldPosition, 1.0f), level.m_viewProjection);
        depth = clip.z;
        return (clip.xy * float2(0.5f, -0.5f) + 0.5f) * kSunShadowVirtualSize;
    }


    float2 ProjectTexel(const SunShadowData data, const uint level, const float3 worldPosition, out float depth)
    {
        return ProjectTexel(data.m_levels.LoadElement(level), worldPosition, depth);
    }


    // The finest level whose texels are no smaller than the camera pixel at this position and that still covers it with a
    // margin for filtering. Marking and sampling must agree, so both call this function.
    Projection Project(const SunShadowData data, const float3 worldPosition, const uint extraBias = 0)
    {
        const float distance = length(worldPosition - data.m_cameraPosition.xyz);
        const float pixelSize = data.m_cameraPosition.w * (data.m_isOrthographicCamera != 0 ? 1.0f : distance);
        const uint levelBias = min((uint)(ceil(log2(max(pixelSize / data.m_level0TexelSize, 1.0f))) + data.m_levelBias) + extraBias,
                                   data.m_levelCount > 0 ? data.m_levelCount - 1 : 0);

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


    //! Whether the page holds this frame's casters and may be sampled.
    bool IsBacked(const uint entry)
    {
        const uint flags = kSunShadowPageAllocated | kSunShadowPageRequested;
        return (entry & (flags | kSunShadowPageUndrawn)) == flags;
    }


    void RequestEntry(RWStructuredBuffer<uint> requests, const uint index, const uint flags, const bool isFeedback)
    {
        const uint address = isFeedback ? index / 32 : index;
        const uint bits = isFeedback ? 1u << (index % 32) : flags;
        if ((requests[address] & bits) != bits)
            InterlockedOr(requests[address], bits);
    }


    void RequestFootprint(RWStructuredBuffer<uint> requests, const uint4 indices, const uint flags, const bool isFeedback)
    {
        RequestEntry(requests, indices.x, flags, isFeedback);
        if (indices.y != indices.x)
            RequestEntry(requests, indices.y, flags, isFeedback);
        if (indices.z != indices.x && indices.z != indices.y)
            RequestEntry(requests, indices.z, flags, isFeedback);
        if (indices.w != indices.x && indices.w != indices.y && indices.w != indices.z)
            RequestEntry(requests, indices.w, flags, isFeedback);
    }


    void RequestProjection(RWStructuredBuffer<uint> requests, const SunShadowData data, const Projection projection,
                           const uint flags, const bool isFeedback)
    {
        if (!projection.m_isValid)
            return;

        const int2 texel = int2(floor(projection.m_texel));
        const int2 origin = data.m_levels.LoadElement(projection.m_level).m_pageOrigin;
        const uint2 lo = uint2(clamp(texel - 3, 0, (int)kSunShadowVirtualSize - 1));
        const uint2 hi = uint2(clamp(texel + 3, 0, (int)kSunShadowVirtualSize - 1));
        const uint4 indices = uint4(GetPageIndex(projection.m_level, origin, lo),
                                    GetPageIndex(projection.m_level, origin, uint2(hi.x, lo.y)),
                                    GetPageIndex(projection.m_level, origin, uint2(lo.x, hi.y)),
                                    GetPageIndex(projection.m_level, origin, hi));
        RequestFootprint(requests, indices, flags, isFeedback);
    }


    void RequestPages(RWStructuredBuffer<uint> pageTable, const SunShadowData data, const float3 worldPosition,
                      const uint extraBias = 0)
    {
        const uint flags = kSunShadowPageRequested | (extraBias != 0 ? kSunShadowPageCoarse : 0);
        RequestProjection(pageTable, data, Project(data, worldPosition, extraBias), flags, false);
    }


    float LoadDepth(const SunShadowBinding binding, const uint entry, const uint2 tap)
    {
        const uint3 atlasTexel = uint3(GetAtlasTexel(entry, tap), 0);
        [branch] if ((entry & kSunShadowPageDynamic) != 0)
            return asfloat(binding.m_atlas.Load(atlasTexel));

        return asfloat(binding.m_staticAtlas.Load(atlasTexel));
    }


    //! The level that the lookups of a position read.
    struct LevelSelection
    {
        SunShadowLevel m_level;
        uint m_index;
        bool m_isBacked;
    };


    //! The level that Project() selects for the position, or the next coarser one whose page under the position is backed.
    //! Pages are requested for this position, so the selection must not use one that is offset from it.
    LevelSelection SelectBackedLevel(const SunShadowBinding binding, const SunShadowData data, const float3 worldPosition)
    {
        LevelSelection result = (LevelSelection)0;
        Projection center = Project(data, worldPosition);
        if (!center.m_isValid)
            return result;

        if (binding.m_feedback.IsValid())
            RequestProjection(binding.m_feedback.Get(), data, center, kSunShadowPageRequested, true);

        // Each level is loaded once and reused by the filter; Project() has already projected the position into the first.
        const uint firstLevel = center.m_level;
        for (uint index = firstLevel; index < data.m_levelCount && !result.m_isBacked; ++index)
        {
            result.m_level = data.m_levels.LoadElement(index);
            result.m_index = index;
            if (index != firstLevel)
                center.m_texel = ProjectTexel(result.m_level, worldPosition, center.m_depth);

            const bool isInside = all(center.m_texel >= 2.0f) && all(center.m_texel <= kSunShadowVirtualSize - 2.0f);
            if (!isInside)
                continue;

            const uint entryIndex = GetPageIndex(index, result.m_level.m_pageOrigin, uint2(center.m_texel));
            result.m_isBacked = IsBacked(binding.m_pageTable.Load(entryIndex));
        }

        return result;
    }


    //! Fraction of the sun visible from the receiver in the selected level. The receiver may be offset along the light from
    //! the position that selected the level, which keeps its texels.
    float FilterLevel(const SunShadowBinding binding, const SunShadowData data, const LevelSelection selection,
                      const float3 receiverPosition, const float3 normal)
    {
        // Offset along the normal by a few texels of the chosen level to avoid self-shadowing.
        const SunShadowLevel level = selection.m_level;
        const float texelSize = level.m_texelSize;
        float receiverDepth;
        const float2 texel = ProjectTexel(level, receiverPosition + normal * (2.0f * texelSize), receiverDepth);
        receiverDepth += texelSize * data.m_depthPerUnit;

        const int2 base = int2(floor(texel));
        const int2 pageOrigin = level.m_pageOrigin;

        // The taps usually fall into one page, which then needs a single page table load.
        const uint2 tapMin = uint2(clamp(base - 1, 0, (int)kSunShadowVirtualSize - 1));
        const uint2 tapMax = uint2(clamp(base + 1, 0, (int)kSunShadowVirtualSize - 1));
        const uint firstEntryIndex = GetPageIndex(selection.m_index, pageOrigin, tapMin);
        const bool isOnePage = firstEntryIndex == GetPageIndex(selection.m_index, pageOrigin, tapMax);
        const uint firstEntry = binding.m_pageTable.Load(firstEntryIndex);

        float visible = 0.0f;
        [unroll] for (int y = -1; y <= 1; ++y)
        {
            [unroll] for (int x = -1; x <= 1; ++x)
            {
                const uint2 tap = uint2(clamp(base + int2(x, y), 0, (int)kSunShadowVirtualSize - 1));
                // Marking requested every page the filter reads; others may hold content cached for other pixels.
                uint entry = firstEntry;
                [branch] if (!isOnePage)
                    entry = binding.m_pageTable.Load(GetPageIndex(selection.m_index, pageOrigin, tap));

                if (!IsBacked(entry))
                {
                    visible += 1.0f;
                    continue;
                }

                // Reverse Z: the occluder is closer to the sun when its depth is larger.
                const float occluderDepth = LoadDepth(binding, entry, tap);
                visible += occluderDepth > receiverDepth ? 0.0f : 1.0f;
            }
        }

        return visible / 9.0f;
    }


    //! Fraction of the sun visible from the position; 1 without shadow data or outside the clip levels.
    float SampleVisibility(const SunShadowBinding binding, const float3 worldPosition, const float3 normal)
    {
        if (!binding.m_data.IsValid())
            return 1.0f;

        const SunShadowData data = binding.m_data.Load();
        const LevelSelection selection = SelectBackedLevel(binding, data, worldPosition);
        if (!selection.m_isBacked)
            return 1.0f;

        return FilterLevel(binding, data, selection, worldPosition, normal);
    }


    //! SampleVisibility for a surface in the camera depth. Shadow casters are drawn at LODs selected for the resolution of
    //! their shadow pages, so close to a surface they differ from the triangles the camera sees. A short ray through the
    //! camera depth finds the occluders within a few shadow texels of the surface, and the shadow lookup starts where the
    //! ray ends. The surface selects the level, as it did when its pages were requested, and the texels of that level give
    //! the ray its length.
    float SampleVisibility(const SunShadowBinding binding, const ScreenTrace::Context trace, const float3 worldPosition,
                           const float3 normal, const float3 directionToLight)
    {
        if (!binding.m_data.IsValid())
            return 1.0f;

        const SunShadowData data = binding.m_data.Load();
        const LevelSelection selection = SelectBackedLevel(binding, data, worldPosition);
        if (!selection.m_isBacked)
            return 1.0f;

        const bool isEnabled = trace.m_isValid && data.m_screenTraceTexels > 0.0f && data.m_isOrthographicCamera == 0;
        if (!isEnabled || dot(normal, directionToLight) <= 0.0f)
            return FilterLevel(binding, data, selection, worldPosition, normal);

        const float pixelSize = data.m_cameraPosition.w * length(worldPosition - data.m_cameraPosition.xyz);
        const float rayLength = selection.m_level.m_texelSize * data.m_screenTraceTexels;
        if (ScreenTrace::IsOccluded(trace, worldPosition + normal * pixelSize, directionToLight, rayLength, pixelSize, 8))
            return 0.0f;

        return FilterLevel(binding, data, selection, worldPosition + directionToLight * rayLength, normal);
    }
} // namespace SunShadow
