#pragma once
#include <Shaders/Base/Base.h>

float4 FrustumPlane(float4x4 localToClip, uint planeIndex)
{
    const float4 x = float4(localToClip[0][0], localToClip[1][0], localToClip[2][0], localToClip[3][0]);
    const float4 y = float4(localToClip[0][1], localToClip[1][1], localToClip[2][1], localToClip[3][1]);
    const float4 z = float4(localToClip[0][2], localToClip[1][2], localToClip[2][2], localToClip[3][2]);
    const float4 w = float4(localToClip[0][3], localToClip[1][3], localToClip[2][3], localToClip[3][3]);
    switch (planeIndex)
    {
    case 0:
        return w + x;
    case 1:
        return w - x;
    case 2:
        return w + y;
    case 3:
        return w - y;
    case 4:
        return z;
    default:
        return w - z;
    }
}


bool VisibleBounds(float3 boundsMin, float3 boundsMax, float4x4 localToClip)
{
    const float3 center = (boundsMin + boundsMax) * 0.5f;
    const float3 extent = (boundsMax - boundsMin) * 0.5f;
    for (uint planeIndex = 0; planeIndex < 6; ++planeIndex)
    {
        const float4 plane = FrustumPlane(localToClip, planeIndex);
        const float tolerance = 1e-4f * length(plane.xyz);
        if (dot(plane, float4(center, 1.0f)) + dot(abs(plane.xyz), extent) < -tolerance)
            return false;
    }
    return true;
}


bool VisibleSphere(float4 sphere, float4x4 localToClip)
{
    for (uint planeIndex = 0; planeIndex < 6; ++planeIndex)
    {
        const float4 plane = FrustumPlane(localToClip, planeIndex);
        if (dot(plane, float4(sphere.xyz, 1.0f)) + (sphere.w + 1e-4f) * length(plane.xyz) < 0.0f)
            return false;
    }
    return true;
}


//! The largest scale a row-vector transform applies to any direction's length.
float MaxAxisScale(float4x4 transform)
{
    const float3 squaredScale = float3(dot(transform[0].xyz, transform[0].xyz),
                                       dot(transform[1].xyz, transform[1].xyz),
                                       dot(transform[2].xyz, transform[2].xyz));
    return sqrt(max(squaredScale.x, max(squaredScale.y, squaredScale.z)));
}


//! A local sphere transformed to world space; the radius grows with the largest axis scale.
float4 TransformSphere(float4 sphere, float4x4 localToWorld)
{
    return float4(mul(float4(sphere.xyz, 1.0f), localToWorld).xyz, sphere.w * MaxAxisScale(localToWorld));
}


struct ScreenBounds
{
    float2 m_uvMin;
    float2 m_uvMax;
    //! The largest reverse-Z depth, i.e. the depth of the nearest point.
    float m_nearestDepth;
};


//! Bound a box on screen. Returns false when the box reaches behind the camera plane and has no finite screen bounds.
bool ProjectBox(float3 center, float3 extent, float4x4 localToClip, out ScreenBounds result)
{
    result = (ScreenBounds)0;
    const float4 base = mul(float4(center, 1.0f), localToClip);
    const float4 axisX = localToClip[0] * extent.x;
    const float4 axisY = localToClip[1] * extent.y;
    const float4 axisZ = localToClip[2] * extent.z;

    float3 ndcMin = 1e30f;
    float3 ndcMax = -1e30f;
    [unroll] for (uint corner = 0; corner < 8; ++corner)
    {
        const float4 clip =
            base + ((corner & 1) ? axisX : -axisX) + ((corner & 2) ? axisY : -axisY) + ((corner & 4) ? axisZ : -axisZ);
        if (clip.w <= 1e-5f)
            return false;

        const float3 ndc = clip.xyz / clip.w;
        ndcMin = min(ndcMin, ndc);
        ndcMax = max(ndcMax, ndc);
    }

    // Normalized device Y points up, texture V points down.
    result.m_uvMin = float2(ndcMin.x, -ndcMax.y) * 0.5f + 0.5f;
    result.m_uvMax = float2(ndcMax.x, -ndcMin.y) * 0.5f + 0.5f;
    result.m_nearestDepth = ndcMax.z;
    return true;
}


//! The pyramid covers the depth target padded to powers of two; m_uvScale maps screen UVs into it.
struct DepthPyramid
{
    Texture2DDescriptor<float> m_texture;
    uint2 m_size;
    float2 m_uvScale;
    uint m_mipCount;
};


//! Test screen bounds against a farthest-depth pyramid; the bounds cover at most 2x2 texels of the selected mip.
//! Regions outside the screen have no depth coverage and are never reported as occluded.
bool IsOccluded(DepthPyramid pyramid, ScreenBounds bounds)
{
    if (any(bounds.m_uvMax < 0.0f) || any(bounds.m_uvMin > 1.0f))
        return false;

    const float2 uvMin = saturate(bounds.m_uvMin) * pyramid.m_uvScale;
    const float2 uvMax = saturate(bounds.m_uvMax) * pyramid.m_uvScale;
    const float2 extent = (uvMax - uvMin) * float2(pyramid.m_size);
    const float level = ceil(log2(max(max(extent.x, extent.y), 1.0f)));
    const uint mip = min(uint(level), pyramid.m_mipCount - 1);
    const uint2 mipSize = max(pyramid.m_size >> mip, 1u);
    const uint2 first = min(uint2(uvMin * float2(mipSize)), mipSize - 1);
    const uint2 last = min(uint2(uvMax * float2(mipSize)), mipSize - 1);

    Texture2D<float> texture = pyramid.m_texture.Get();
    const float depth0 = texture.Load(uint3(first.x, first.y, mip));
    const float depth1 = texture.Load(uint3(last.x, first.y, mip));
    const float depth2 = texture.Load(uint3(first.x, last.y, mip));
    const float depth3 = texture.Load(uint3(last.x, last.y, mip));
    const float farthest = min(min(depth0, depth1), min(depth2, depth3));
    return bounds.m_nearestDepth < farthest;
}
