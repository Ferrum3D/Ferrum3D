#pragma once
#include <Shaders/Base/Packing.hlsli>
#include <Shaders/Deferred/Contracts.h>
#include <Shaders/Shading/Color.hlsli>

namespace GBuffer
{
    struct Packed
    {
        uint m_g0;
        uint m_g1;
        uint m_g2;
        uint m_g3;
    };


    struct Unpacked
    {
        float3 m_color;
        float3 m_normal;
        float m_roughness;
        float m_metallic;
        float m_ambientOcclusion;
        float3 m_emissive;
        uint m_shadingRoute;
    };


    uint GetShadingRoute(const uint g0)
    {
        return g0 >> 24;
    }


    bool HasExtendedPayload(const uint g2)
    {
        return ((g2 >> 24) & kExtendedPayloadValid) != 0;
    }


    Packed Encode(const Unpacked source)
    {
        const float3 srgb = Shading::LinearToSRGB(saturate(source.m_color));
        const uint3 color = uint3(round(srgb * 255));
        const float3 properties = saturate(float3(source.m_roughness, source.m_metallic, source.m_ambientOcclusion));
        const uint3 propertyBytes = uint3(round(properties * 255));
        const float3 emissive = max(select(isfinite(source.m_emissive), source.m_emissive, 0.0f), 0.0f);
        const uint flags = any(emissive > 0) ? kExtendedPayloadValid : 0;

        Packed result;
        result.m_g0 = Math::Pack::RGBA32UintToRGBA8Uint(uint4(color, source.m_shadingRoute));
        result.m_g1 = Math::Pack::NormalToOctahedral16Snorm(source.m_normal);
        result.m_g2 = Math::Pack::RGBA32UintToRGBA8Uint(uint4(propertyBytes, flags));
        result.m_g3 = flags != 0 ? Math::Pack::RGB32FloatToR11G11B10Float(emissive) : 0;
        return result;
    }


    Unpacked Decode(const Packed source)
    {
        const float3 srgb = Math::Pack::RGBA8UnormToRGBA32Float(source.m_g0).rgb;
        const float3 properties = Math::Pack::RGBA8UnormToRGBA32Float(source.m_g2).rgb;

        Unpacked result;
        result.m_color = Shading::SRGBToLinear(srgb);
        result.m_normal = Math::Pack::Octahedral16SnormToNormal(source.m_g1);
        result.m_roughness = properties.x;
        result.m_metallic = properties.y;
        result.m_ambientOcclusion = properties.z;
        result.m_emissive = HasExtendedPayload(source.m_g2) ? Math::Pack::R11G11B10FloatToRGB32Float(source.m_g3) : 0;
        result.m_shadingRoute = GetShadingRoute(source.m_g0);
        return result;
    }
} // namespace GBuffer
