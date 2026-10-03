#pragma once

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
