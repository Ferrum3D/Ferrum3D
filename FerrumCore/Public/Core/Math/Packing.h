#pragma once
#include <Core/Math/Vector2.h>
#include <Core/Math/Vector3.h>
#include <Core/Math/Vector4.h>
#include <cmath>

namespace FE::Math::Pack
{
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint16_t R32FloatToR16Float(const float source)
    {
        const __m128i v = _mm_cvtps_ph(_mm_set_ss(source), _MM_FROUND_TO_NEAREST_INT);
        return static_cast<uint16_t>(_mm_cvtsi128_si32(v));
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float R16FloatToR32Float(const uint16_t source)
    {
        const __m128i v = _mm_cvtsi32_si128(source);
        return _mm_cvtss_f32(_mm_cvtph_ps(v));
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2UInt FE_VECTORCALL RGBA32FloatToRGBA16Float(const Vector4 source)
    {
        const __m128i v = _mm_cvtps_ph(source.m_simdVector, _MM_FROUND_TO_NEAREST_INT);
        const uint64_t qword = _mm_cvtsi128_si64(v);
        return Vector2UInt{ static_cast<uint32_t>(qword), static_cast<uint32_t>(qword >> 32) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL RG32FloatToRG16Float(const Vector2 source)
    {
        const __m128 floatVector = _mm_setr_ps(source.x, source.y, 0.0f, 0.0f);
        const __m128i intVector = _mm_cvtps_ph(floatVector, _MM_FROUND_TO_NEAREST_INT);
        return _mm_cvtsi128_si32(intVector);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL RGBA8UnormToRGBA32Float(const uint32_t source)
    {
        const uint8_t r = (source >> 0) & 0xff;
        const uint8_t g = (source >> 8) & 0xff;
        const uint8_t b = (source >> 16) & 0xff;
        const uint8_t a = (source >> 24) & 0xff;

        const __m128i intVector = _mm_setr_epi32(r, g, b, a);
        const __m128 floatVector = _mm_cvtepi32_ps(intVector);
        return Vector4{ _mm_mul_ps(floatVector, _mm_set1_ps(1.0f / 255.0f)) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL RGBA32FloatToRGBA8Unorm(const Vector4 source)
    {
        const __m128 t = _mm_mul_ps(source.m_simdVector, _mm_set1_ps(255.0f));
        const __m128i v = _mm_shuffle_epi8(_mm_cvtps_epi32(t), _mm_set1_epi32(0x0C080400));
        return _mm_cvtsi128_si32(v);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t RGBA32FloatToA2R10G10B10Unorm(const Vector4 source)
    {
        const __m128 kScale = _mm_setr_ps(1023.0f, //
                                          1023.0f * 1024.0f * 0.5f,
                                          1023.0f * 1024.0f * 1024.0f,
                                          3.0f * 1024.0f * 1024.0f * 1024.0f * 0.5f);

        const __m128i kMask = _mm_setr_epi32(0x3FF, 0x3FF << (10 - 1), 0x3FF << 20, 0x3 << (30 - 1));

        __m128 vResult = source.m_simdVector;
        // Scale by multiplication
        vResult = _mm_mul_ps(vResult, kScale);
        // Convert to int
        __m128i vResulti = _mm_cvttps_epi32(vResult);
        // Mask off any fraction
        vResulti = _mm_and_si128(vResulti, kMask);
        // Do a horizontal or of 4 entries
        __m128i vResulti2 = _mm_shuffle_epi32(vResulti, _MM_SHUFFLE(3, 2, 3, 2));
        // x = x|z, y = y|w
        vResulti = _mm_or_si128(vResulti, vResulti2);
        // Move Z to the x position
        vResulti2 = _mm_shuffle_epi32(vResulti, _MM_SHUFFLE(1, 1, 1, 1));
        // Perform a left shift by one bit on y|w
        vResulti2 = _mm_add_epi32(vResulti2, vResulti2);
        // i = x|y|z|w
        vResulti = _mm_or_si128(vResulti, vResulti2);
        return _mm_cvtsi128_si32(vResulti);
    }


    FE_FORCE_INLINE uint32_t NormalToOctahedral16Snorm(const Vector3 normal)
    {
        const Vector3 n = normal / (std::abs(normal.x) + std::abs(normal.y) + std::abs(normal.z));
        const float x = n.z >= 0 ? n.x : (1 - std::abs(n.y)) * (n.x >= 0 ? 1.0f : -1.0f);
        const float y = n.z >= 0 ? n.y : (1 - std::abs(n.x)) * (n.y >= 0 ? 1.0f : -1.0f);
        const Vector3 rounded = Math::Round(Vector3(x, y, 0) * 32767.0f);
        return (static_cast<uint32_t>(static_cast<int32_t>(rounded.x)) & 65535u)
            | (static_cast<uint32_t>(static_cast<int32_t>(rounded.y)) << 16);
    }


    FE_FORCE_INLINE Vector3 Octahedral16SnormToNormal(const uint32_t word)
    {
        const int32_t sx = static_cast<int32_t>(word & 32767u) - static_cast<int32_t>(word & 32768u);
        const int32_t sy = static_cast<int32_t>((word >> 16) & 32767u) - static_cast<int32_t>((word >> 16) & 32768u);
        const float x = sx / 32767.0f;
        const float y = sy / 32767.0f;
        const float z = 1 - std::abs(x) - std::abs(y);
        const Vector3 normal = z >= 0
            ? Vector3(x, y, z)
            : Vector3((1 - std::abs(y)) * (x >= 0 ? 1.0f : -1.0f), (1 - std::abs(x)) * (y >= 0 ? 1.0f : -1.0f), z);
        return Math::Normalize(normal);
    }


    FE_FORCE_INLINE uint32_t EncodeUnsignedFloat(float value, const uint32_t mantissaBits)
    {
        const float maximum = mantissaBits == 6 ? 65024.0f : 64512.0f;
        value = std::isfinite(value) ? Math::Clamp(value, 0.0f, maximum) : 0.0f;
        const uint32_t halfBits = R32FloatToR16Float(value);
        const uint32_t shift = 10 - mantissaBits;
        const uint32_t rounded = halfBits + ((1u << (shift - 1)) - 1) + ((halfBits >> shift) & 1);
        return rounded >> shift;
    }


    FE_FORCE_INLINE uint32_t RGB32FloatToR11G11B10Float(const Vector3 source)
    {
        const uint32_t red = EncodeUnsignedFloat(source.x, 6);
        const uint32_t green = EncodeUnsignedFloat(source.y, 6);
        const uint32_t blue = EncodeUnsignedFloat(source.z, 5);
        return red | (green << 11) | (blue << 22);
    }


    FE_FORCE_INLINE Vector3 R11G11B10FloatToRGB32Float(const uint32_t word)
    {
        return Vector3(R16FloatToR32Float(static_cast<uint16_t>((word & 2047) << 4)),
                       R16FloatToR32Float(static_cast<uint16_t>(((word >> 11) & 2047) << 4)),
                       R16FloatToR32Float(static_cast<uint16_t>((word >> 22) << 5)));
    }
} // namespace FE::Math::Pack
