#pragma once
#include <FeCore/Math/Vector4.h>

namespace FE
{
    struct Color4F final
    {
        static constexpr float kMaxByte = 255.0f;
        static constexpr float kInvMaxByte = 1.0f / 255.0f;

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

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F(float value)
            : m_simdVector(_mm_set1_ps(value))
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F(__m128 vec)
            : m_simdVector(vec)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F(Vector4F vec)
            : m_simdVector(vec.m_simdVector)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F(float r, float g, float b, float a)
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

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Color4F FE_VECTORCALL FromBytes(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        {
            const __m128i intVector = _mm_setr_epi32(r, g, b, a);
            const __m128 floatVector = _mm_cvtepi32_ps(intVector);
            return Color4F{ _mm_mul_ps(floatVector, _mm_set1_ps(kInvMaxByte)) };
        }

        template<Math::Swizzle TSwizzle>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Color4F FE_VECTORCALL Swizzle(Color4F color)
        {
            return Color4F{ _mm_shuffle_ps(color.m_simdVector, color.m_simdVector, festd::to_underlying(TSwizzle)) };
        }
    };


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator+(Color4F lhs, Color4F rhs)
    {
        return Color4F{ _mm_add_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator-(Color4F lhs, Color4F rhs)
    {
        return Color4F{ _mm_sub_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator*(Color4F lhs, Color4F rhs)
    {
        return Color4F{ _mm_mul_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator/(Color4F lhs, Color4F rhs)
    {
        return Color4F{ _mm_div_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator*(Color4F lhs, float rhs)
    {
        return Color4F{ _mm_mul_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL operator/(Color4F lhs, float rhs)
    {
        return Color4F{ _mm_div_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    namespace Math
    {
        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL Clamp<Color4F>(Color4F vec, Color4F min, Color4F max)
        {
            return Color4F{ _mm_max_ps(min.m_simdVector, _mm_min_ps(vec.m_simdVector, max.m_simdVector)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Color4F FE_VECTORCALL Saturate(Color4F vec)
        {
            return Color4F{ _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(vec.m_simdVector, _mm_set1_ps(1.0f))) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpEqualMask(Color4F lhs, Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpeq_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpNotEqualMask(Color4F lhs, Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpneq_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessMask(Color4F lhs, Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmplt_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterMask(Color4F lhs, Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpgt_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessEqualMask(Color4F lhs, Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmple_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterEqualMask(Color4F lhs, Color4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpge_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EqualEstimate(Color4F lhs, Color4F rhs,
                                                                               float epsilon = Constants::Epsilon)
        {
            const __m128 kSignMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
            const __m128 distance = _mm_and_ps(_mm_sub_ps(lhs.m_simdVector, rhs.m_simdVector), kSignMask);
            const uint32_t mask = _mm_movemask_ps(_mm_cmpgt_ps(distance, _mm_set1_ps(epsilon)));
            return mask == 0;
        }
    } // namespace Math


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator==(Color4F lhs, Color4F rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) == 0;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator!=(Color4F lhs, Color4F rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) != 0;
    }
} // namespace FE
