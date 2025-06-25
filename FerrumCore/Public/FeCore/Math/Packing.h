#pragma once
#include <FeCore/Math/Vector2.h>
#include <FeCore/Math/Vector4.h>

namespace FE::Math::Pack
{
    namespace Internal
    {
        union FP32
        {
            uint32_t u;
            float f;
            struct
            {
                uint32_t Mantissa : 23;
                uint32_t Exponent : 8;
                uint32_t Sign : 1;
            };
        };

        union FP16
        {
            uint16_t u;
            struct
            {
                uint32_t Mantissa : 10;
                uint32_t Exponent : 5;
                uint32_t Sign : 1;
            };
        };
    } // namespace Internal


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint16_t R32FloatToR16Float(const float x)
    {
        using namespace Internal;

        //
        // float_to_half_fast2 from https://gist.github.com/rygorous/2156668
        //

        FP32 f;
        f.f = x;

        FP32 infty = { 31 << 23 };
        FP32 magic = { 15 << 23 };
        FP16 o = { 0 };

        uint32_t sign = f.Sign;
        f.Sign = 0;

        // Based on ISPC reference code (with minor modifications)
        if (f.Exponent == 255) // Inf or NaN (all exponent bits set)
        {
            o.Exponent = 31;
            o.Mantissa = f.Mantissa ? 0x200 : 0; // NaN->qNaN and Inf->Inf
        }
        else // (De)normalized number or zero
        {
            f.u &= ~0xfff; // Make sure we don't get sticky bits
            // Not necessarily the best move in terms of accuracy, but matches behavior
            // of other versions.

            // Shift exponent down, denormalize if necessary.
            // NOTE This represents half-float denormals using single precision denormals.
            // The main reason to do this is that there's no shift with per-lane variable
            // shifts in SSE*, which we'd otherwise need. It has some funky side effects
            // though:
            // - This conversion will actually respect the FTZ (Flush To Zero) flag in
            //   MXCSR - if it's set, no half-float denormals will be generated. I'm
            //   honestly not sure whether this is good or bad. It's definitely interesting.
            // - If the underlying HW doesn't support denormals (not an issue with Intel
            //   CPUs, but might be a problem on GPUs or PS3 SPUs), you will always get
            //   flush-to-zero behavior. This is bad, unless you're on a CPU where you don't
            //   care.
            // - Denormals tend to be slow. FP32 denormals are rare in practice outside of things
            //   like recursive filters in DSP - not a typical half-float application. Whether
            //   FP16 denormals are rare in practice, I don't know. Whatever slow path your HW
            //   may or may not have for denormals, this may well hit it.
            f.f *= magic.f;

            f.u += 0x1000; // Rounding bias
            if (f.u > infty.u)
                f.u = infty.u; // Clamp to signed infinity if overflowed

            o.u = static_cast<uint16_t>(f.u >> 13); // Take the bits!
        }

        o.Sign = sign;
        return o.u;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2UInt FE_VECTORCALL RGBA32FloatToRGBA16Float(const Vector4 f)
    {
        //
        // float_to_half_SSE2 from https://gist.github.com/rygorous/2156668
        //

        const __m128i mask_sign = _mm_set1_epi32(0x80000000u);
        const __m128i mask_round = _mm_set1_epi32(~0xfffu);
        const __m128i c_f32infty = _mm_set1_epi32(255 << 23);
        const __m128i c_magic = _mm_set1_epi32(15 << 23);
        const __m128i c_nanbit = _mm_set1_epi32(0x200);
        const __m128i c_infty_as_fp16 = _mm_set1_epi32(0x7c00);
        const __m128i c_clamp = _mm_set1_epi32((31 << 23) - 0x1000);

        const __m128 msign = _mm_castsi128_ps(mask_sign);
        const __m128 justsign = _mm_and_ps(msign, f.m_simdVector);
        const __m128i f32infty = c_f32infty;
        const __m128 absf = _mm_xor_ps(f.m_simdVector, justsign);
        const __m128 mround = _mm_castsi128_ps(mask_round);
        const __m128i absf_int = _mm_castps_si128(absf); // pseudo-op, but val needs to be copied once so count as mov
        const __m128i b_isnan = _mm_cmpgt_epi32(absf_int, f32infty);
        const __m128i b_isnormal = _mm_cmpgt_epi32(f32infty, _mm_castps_si128(absf));
        const __m128i nanbit = _mm_and_si128(b_isnan, c_nanbit);
        const __m128i inf_or_nan = _mm_or_si128(nanbit, c_infty_as_fp16);

        const __m128 fnosticky = _mm_and_ps(absf, mround);
        const __m128 scaled = _mm_mul_ps(fnosticky, _mm_castsi128_ps(c_magic));
        const __m128 clamped = _mm_min_ps(
            scaled, _mm_castsi128_ps(c_clamp)); // logically, we want PMINSD on "biased", but this should gen better code
        const __m128i biased = _mm_sub_epi32(_mm_castps_si128(clamped), _mm_castps_si128(mround));
        const __m128i shifted = _mm_srli_epi32(biased, 13);
        const __m128i normal = _mm_and_si128(shifted, b_isnormal);
        const __m128i not_normal = _mm_andnot_si128(b_isnormal, inf_or_nan);
        const __m128i joined = _mm_or_si128(normal, not_normal);

        const __m128i sign_shift = _mm_srli_epi32(_mm_castps_si128(justsign), 16);

        const __m128i t = _mm_or_si128(joined, sign_shift);
        const __m128i v = _mm_shuffle_epi8(t, _mm_set_epi32(-1, -1, 0x0D0C0908, 0x05040100));

        const uint32_t resultX = _mm_cvtsi128_si32(v);
        const uint32_t resultY = _mm_cvtsi128_si32(_mm_shuffle_epi32(v, _MM_SHUFFLE(1, 1, 1, 1)));

        // ~20 SSE2 ops
        return Vector2UInt{ resultX, resultY };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL RG32FloatToRG16Float(const Vector2 source)
    {
        const Vector4 source4{ source.x, source.y, 0.0f, 0.0f };
        return RGBA32FloatToRGBA16Float(source4).x;
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


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t RGB32FloatToA2R10G10B10(const Vector3 source)
    {
        const __m128 t = _mm_castsi128_ps(_mm_cvttps_epi32(_mm_mul_ps(source.m_simdVector, _mm_set1_ps(1023.0f))));
        const __m128i v = _mm_castps_si128(_mm_and_ps(t, _mm_castsi128_ps(_mm_set1_epi32(0x3ff))));

        const uint32_t r = _mm_extract_epi32(v, 0);
        const uint32_t g = _mm_extract_epi32(v, 1);
        const uint32_t b = _mm_extract_epi32(v, 2);
        return (b << 20) | (g << 10) | r;
    }
} // namespace FE::Math::Pack
