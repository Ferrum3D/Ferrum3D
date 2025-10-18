#pragma once
#include <FeCore/Math/Vector2.h>
#include <FeCore/Math/Vector4.h>

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
} // namespace FE::Math::Pack
