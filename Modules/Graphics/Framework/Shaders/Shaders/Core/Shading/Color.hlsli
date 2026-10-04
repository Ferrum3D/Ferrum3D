#pragma once
#include <Shaders/Base/Base.hlsli>

namespace Shading
{
    float3 LinearToSRGB(const float3 color)
    {
        const float3 low = color * 12.92f;
        const float3 high = 1.055f * pow(max(color, 0.0f), 1.0f / 2.4f) - 0.055f;
        return select(color <= 0.0031308f, low, high);
    }


    // Narkowicz's ACES filmic fit, applied to pre-exposed linear RGB.
    float3 TonemapACES(const float3 color)
    {
        const float3 x = max(color, 0.0f);
        return saturate((x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f));
    }
} // namespace Shading
