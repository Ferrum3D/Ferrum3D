#pragma once
#include <Shaders/Base/BaseTypes.hlsli>

namespace Math
{
    static const float kPI = 3.14159265358979323f;


    template<typename T>
    T Min3(const T a, const T b, const T c)
    {
        return min(a, min(b, c));
    }


    template<typename T>
    T Max3(const T a, const T b, const T c)
    {
        return max(a, max(b, c));
    }


    float3 LinearToSRGB(const float3 color)
    {
        const float3 low = color * 12.92f;
        const float3 high = 1.055f * pow(max(color, 0.0f), 1.0f / 2.4f) - 0.055f;
        return select(color <= 0.0031308f, low, high);
    }


    float3 SRGBToLinear(const float3 color)
    {
        const float3 low = color / 12.92f;
        const float3 high = pow((max(color, 0.0f) + 0.055f) / 1.055f, 2.4f);
        return select(color <= 0.04045f, low, high);
    }


    // Gamma 2.2 approximations for storage where the exact sRGB transfer is unnecessary.
    float3 LinearToSRGBApprox(const float3 color)
    {
        return pow(max(color, 0.0f), 1.0f / 2.2f);
    }


    float3 SRGBToLinearApprox(const float3 color)
    {
        return pow(max(color, 0.0f), 2.2f);
    }
} // namespace Math
