#pragma once
#include <Shaders/Shadows/SunShadow.hlsli>

bool ProjectPageRegion(const SunShadowBlock block, const float3 regionMin, const float3 regionMax,
                       out uint2 pageMin, out uint2 pageCount)
{
    pageMin = 0;
    pageCount = 0;
    const float virtualSize = float(block.m_pagesPerAxis * kSunShadowPageSize);
    float2 texelMin = 1e30f;
    float2 texelMax = -1e30f;
    bool isBehind = false;
    [unroll] for (uint corner = 0; corner < 8; ++corner)
    {
        const float3 position = float3((corner & 1) ? regionMax.x : regionMin.x,
                                       (corner & 2) ? regionMax.y : regionMin.y,
                                       (corner & 4) ? regionMax.z : regionMin.z);
        const float4 clip = mul(float4(position, 1.0f), block.m_viewProjection);
        isBehind = isBehind || clip.w <= 0.0f;
        const float2 texel = (clip.xy / max(clip.w, 1e-6f) * float2(0.5f, -0.5f) + 0.5f) * virtualSize;
        texelMin = min(texelMin, texel);
        texelMax = max(texelMax, texel);
    }

    // Bounds that reach behind a perspective map cover an unbounded part of it, so the whole map is freed.
    if (isBehind && block.m_isPerspective != 0)
    {
        texelMin = 0.0f;
        texelMax = virtualSize - 1.0f;
    }

    if (any(texelMax < 0.0f) || any(texelMin >= virtualSize))
        return false;

    const float lastPage = float(block.m_pagesPerAxis - 1);
    pageMin = uint2(clamp(floor(texelMin / kSunShadowPageSize), 0.0f, lastPage));
    const uint2 pageMax = uint2(clamp(floor(texelMax / kSunShadowPageSize), 0.0f, lastPage));
    pageCount = pageMax - pageMin + 1;
    return true;
}
