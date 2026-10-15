#pragma once
#include <Shaders/Base/BaseTypes.hlsli>

namespace Math
{
    namespace Pack
    {
        //
        // 16-bit Float
        //


        uint2 RGBA32FloatToRGBA16Float(const float4 source)
        {
            return uint2((f32tof16(source.x) & 0xffff) | (f32tof16(source.y) << 16),
                         (f32tof16(source.z) & 0xffff) | (f32tof16(source.w) << 16));
        }


        uint2 RGBA32FloatToRGBA16Float(const fehalf4 source)
        {
            // Things like this should in theory be optimized by drivers to something like a bit-cast.
            return uint2((f32tof16(source.x) & 0xffff) | (f32tof16(source.y) << 16),
                         (f32tof16(source.z) & 0xffff) | (f32tof16(source.w) << 16));
        }


        float4 RGBA16FloatToRGBA32Float(const uint2 source)
        {
            return float4(f16tof32(source.x), f16tof32(source.x >> 16), f16tof32(source.y), f16tof32(source.y >> 16));
        }


        fehalf4 RGBA16FloatToRGBA32Float_Half(const uint2 source)
        {
            return fehalf4(f16tof32(source.x), f16tof32(source.x >> 16), f16tof32(source.y), f16tof32(source.y >> 16));
        }


        uint RG32FloatToRG16Float(const float2 source)
        {
            return (f32tof16(source.x) & 0xffff) | (f32tof16(source.y) << 16);
        }


        float2 RG16FloatToRG32Float(const uint source)
        {
            return float2(f16tof32(source), f16tof32(source >> 16));
        }


        //
        // 8-bit components
        //


        uint32_t RGBA32UintToRGBA8Uint(const uint4 source)
        {
            const uint4 bytes = source & 0xff;
            return bytes.x | (bytes.y << 8) | (bytes.z << 16) | (bytes.w << 24);
        }


        float4 RGBA8UnormToRGBA32Float(const uint32_t source)
        {
            return float4(((source >> 0) & 0xff), ((source >> 8) & 0xff), ((source >> 16) & 0xff), ((source >> 24) & 0xff))
                / 255.0f;
        }


        uint32_t RGBA32FloatToRGBA8Unorm(const float4 source)
        {
            const float4 t = source * 255.0f;
            return RGBA32UintToRGBA8Uint(uint4(t));
        }


        //
        // A2B10G10R10 Unorm
        //


        uint32_t RGBA32FloatToA2R10G10B10Unorm(const float4 source)
        {
            const float4 t = source * float4(1023.0f, 1023.0f, 1023.0f, 3.0f);
            return (uint32_t(t.x) & 0x3ff) | ((uint32_t(t.y) << 10) & 0x3ff) | ((uint32_t(t.z) << 20) & 0x3ff)
                | (uint32_t(t.w) << 30);
        }


        float4 A2R10G10B10UnormToRGBA32Float(const uint32_t source)
        {
            const float4 t = float4(source & 0x3ff, (source >> 10) & 0x3ff, (source >> 20) & 0x3ff, source >> 30);
            return t / float4(1023.0f, 1023.0f, 1023.0f, 3.0f);
        }


        float2 SignNotZero(float2 value)
        {
            return select(value >= 0, 1.0f, -1.0f);
        }


        uint NormalToOctahedral16Snorm(float3 normal)
        {
            normal /= dot(abs(normal), 1.0f);
            const float2 oct = normal.z >= 0 ? normal.xy : (1 - abs(normal.yx)) * SignNotZero(normal.xy);
            const int2 bits = int2(round(clamp(oct, -1.0f, 1.0f) * 32767));
            return (uint(bits.x) & 65535) | (uint(bits.y) << 16);
        }


        float3 Octahedral16SnormToNormal(uint word)
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


        uint RGB32FloatToR11G11B10Float(const float3 source)
        {
            const uint red = EncodeUnsignedFloat(source.x, 6);
            const uint green = EncodeUnsignedFloat(source.y, 6);
            const uint blue = EncodeUnsignedFloat(source.z, 5);
            return red | (green << 11) | (blue << 22);
        }


        float3 R11G11B10FloatToRGB32Float(uint word)
        {
            return float3(f16tof32((word & 2047) << 4), f16tof32(((word >> 11) & 2047) << 4), f16tof32((word >> 22) << 5));
        }
    } // namespace Pack
} // namespace Math
