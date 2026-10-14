#pragma once
#include <Shaders/Core/Shading/Color.hlsli>
#include <Shaders/Passes/Visibility/Contracts.h>

float3 DecodeSurfaceColor(uint word)
{
    const float3 srgb = float3(word & 255, (word >> 8) & 255, (word >> 16) & 255) / 255.0f;
    return Shading::SRGBToLinear(srgb);
}


uint EncodeSurfaceColor(float3 color, uint route)
{
    const uint3 bytes = uint3(round(Shading::LinearToSRGB(saturate(color)) * 255.0f));
    return bytes.x | (bytes.y << 8) | (bytes.z << 16) | (route << 24);
}


float2 SignNotZero(float2 value)
{
    return select(value >= 0, 1.0f, -1.0f);
}


uint EncodeSurfaceNormal(float3 normal)
{
    normal /= dot(abs(normal), 1.0f);
    const float2 oct = normal.z >= 0 ? normal.xy : (1 - abs(normal.yx)) * SignNotZero(normal.xy);
    const int2 bits = int2(round(clamp(oct, -1.0f, 1.0f) * 32767));
    return (uint(bits.x) & 65535) | (uint(bits.y) << 16);
}


float3 DecodeSurfaceNormal(uint word)
{
    const float2 oct = float2(int(word << 16) >> 16, int(word) >> 16) / 32767.0f;
    float3 normal = float3(oct, 1 - abs(oct.x) - abs(oct.y));
    if (normal.z < 0)
        normal.xy = (1 - abs(normal.yx)) * SignNotZero(normal.xy);

    return normalize(normal);
}


uint EncodeUnsignedFloat(float value, uint mantissaBits)
{
    const float maximum = mantissaBits == 6 ? 65024.0f : 64512.0f;
    value = isfinite(value) ? clamp(value, 0, maximum) : 0;
    const uint halfBits = f32tof16(value);
    const uint shift = 10 - mantissaBits;
    const uint rounded = halfBits + ((1u << (shift - 1)) - 1) + ((halfBits >> shift) & 1);
    return rounded >> shift;
}


uint EncodeSurfaceEmissive(float3 emissive)
{
    return EncodeUnsignedFloat(emissive.x, 6) | (EncodeUnsignedFloat(emissive.y, 6) << 11)
        | (EncodeUnsignedFloat(emissive.z, 5) << 22);
}


float3 DecodeSurfaceEmissive(uint word)
{
    return float3(f16tof32((word & 2047) << 4), f16tof32(((word >> 11) & 2047) << 4), f16tof32((word >> 22) << 5));
}


uint EncodeSurfaceProperties(float roughness, float metallic, float ao, uint flags)
{
    const uint3 bytes = uint3(round(saturate(float3(roughness, metallic, ao)) * 255));
    return bytes.x | (bytes.y << 8) | (bytes.z << 16) | (flags << 24);
}
