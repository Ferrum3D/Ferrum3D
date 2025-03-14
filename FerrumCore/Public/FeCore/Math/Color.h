#pragma once
#include <FeCore/Math/Vector4.h>

namespace FE
{
    struct Color4F final
    {
        union
        {
            __m128 m_simdVector;
            float m_values[4];
            struct
            {
                float r, g, b, a;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F(const float value)
            : m_simdVector(_mm_set1_ps(value))
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F(const __m128 vec)
            : m_simdVector(vec)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F(const Vector4F vec)
            : m_simdVector(vec.m_simdVector)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F(const float r, const float g, const float b, const float a)
            : m_simdVector(_mm_setr_ps(r, g, b, a))
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Color4F FE_VECTORCALL Zero()
        {
            return Color4F{ _mm_setzero_ps() };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float* FE_VECTORCALL Data()
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE const float* FE_VECTORCALL Data() const
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Color4F FE_VECTORCALL LoadUnaligned(const float* values)
        {
            return Color4F{ _mm_loadu_ps(values) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Color4F FE_VECTORCALL LoadAligned(const float* values)
        {
            FE_AssertDebug((reinterpret_cast<uintptr_t>(values) & 15) == 0);
            return Color4F{ _mm_load_ps(values) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Color4F FE_VECTORCALL FromBytes(const uint8_t r, const uint8_t g,
                                                                                     const uint8_t b, const uint8_t a)
        {
            const __m128i intVector = _mm_setr_epi32(r, g, b, a);
            const __m128 floatVector = _mm_cvtepi32_ps(intVector);
            return Color4F{ _mm_mul_ps(floatVector, _mm_set1_ps(1.0f / 255.0f)) };
        }

        template<Math::Swizzle TSwizzle>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Color4F FE_VECTORCALL Swizzle(const Color4F color)
        {
            return Color4F{ _mm_shuffle_ps(color.m_simdVector, color.m_simdVector, festd::to_underlying(TSwizzle)) };
        }
    };


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator+(const Color4F lhs, const Color4F rhs)
    {
        return Color4F{ _mm_add_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator-(const Color4F lhs, const Color4F rhs)
    {
        return Color4F{ _mm_sub_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator*(const Color4F lhs, const Color4F rhs)
    {
        return Color4F{ _mm_mul_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator/(const Color4F lhs, const Color4F rhs)
    {
        return Color4F{ _mm_div_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator*(const Color4F lhs, const float rhs)
    {
        return Color4F{ _mm_mul_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator/(const Color4F lhs, const float rhs)
    {
        return Color4F{ _mm_div_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    namespace Math
    {
        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL Clamp<Color4F>(const Color4F vec, const Color4F min,
                                                                                   const Color4F max)
        {
            return Color4F{ _mm_max_ps(min.m_simdVector, _mm_min_ps(vec.m_simdVector, max.m_simdVector)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL Saturate(const Color4F vec)
        {
            return Color4F{ _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(vec.m_simdVector, _mm_set1_ps(1.0f))) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpEqualMask(const Color4F lhs, const Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpeq_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpNotEqualMask(const Color4F lhs, const Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpneq_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessMask(const Color4F lhs, const Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmplt_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterMask(const Color4F lhs, const Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpgt_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessEqualMask(const Color4F lhs, const Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmple_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterEqualMask(const Color4F lhs, const Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpge_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EqualEstimate(const Color4F lhs, const Color4F rhs,
                                                                               const float epsilon = Constants::kEpsilon)
        {
            const __m128 kSignMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
            const __m128 distance = _mm_and_ps(_mm_sub_ps(lhs.m_simdVector, rhs.m_simdVector), kSignMask);
            const uint32_t mask = _mm_movemask_ps(_mm_cmpgt_ps(distance, _mm_set1_ps(epsilon)));
            return mask == 0;
        }


        namespace Pack
        {
            FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL RGBA8UnormToRGBA32Float(const uint32_t source)
            {
                const uint8_t r = (source >> 16) & 0xff;
                const uint8_t g = (source >> 8) & 0xff;
                const uint8_t b = (source >> 0) & 0xff;
                const uint8_t a = (source >> 24) & 0xff;

                const __m128i intVector = _mm_setr_epi32(r, g, b, a);
                const __m128 floatVector = _mm_cvtepi32_ps(intVector);
                return Color4F{ _mm_mul_ps(floatVector, _mm_set1_ps(1.0f / 255.0f)) };
            }


            FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL RGBA32FloatToRGBA8Unorm(const Color4F source)
            {
                const __m128i t = _mm_cvttps_epi32(_mm_mul_ps(source.m_simdVector, _mm_set1_ps(255.0f)));
                const __m128i v = _mm_min_epi32(t, _mm_set1_epi32(255));
                const uint32_t r = static_cast<uint32_t>(_mm_extract_epi8(v, 4 * 0));
                const uint32_t g = static_cast<uint32_t>(_mm_extract_epi8(v, 4 * 1));
                const uint32_t b = static_cast<uint32_t>(_mm_extract_epi8(v, 4 * 2));
                const uint32_t a = static_cast<uint32_t>(_mm_extract_epi8(v, 4 * 3));
                return (r << 16) | (g << 8) | (b << 0) | (a << 24);
            }
        } // namespace Pack
    } // namespace Math


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator==(const Color4F lhs, const Color4F rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) == 0;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator!=(const Color4F lhs, const Color4F rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) != 0;
    }
} // namespace FE
