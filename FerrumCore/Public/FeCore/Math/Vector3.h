#pragma once
#include <FeCore/Base/BaseMath.h>
#include <FeCore/Logging/Trace.h>
#include <FeCore/SIMD/Utils.h>

namespace FE
{
    //! @brief 3-dimensional vector.
    struct Vector3F final
    {
        union
        {
            __m128 m_simdVector;
            float m_values[3];
            struct
            {
                float x, y, z;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F(float value)
            : m_simdVector(_mm_set1_ps(value))
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F(__m128 vec)
            : m_simdVector(vec)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F(float x, float y, float z)
            : m_simdVector(_mm_setr_ps(x, y, z, 0.0f))
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float* FE_VECTORCALL Data()
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE const float* FE_VECTORCALL Data() const
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3F FE_VECTORCALL Zero()
        {
            return Vector3F{ _mm_setzero_ps() };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3F FE_VECTORCALL LoadUnaligned(const float* values)
        {
            return Vector3F{ _mm_loadu_ps(values) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3F FE_VECTORCALL LoadAligned(const float* values)
        {
            FE_AssertDebug((reinterpret_cast<uintptr_t>(values) & 15) == 0);
            return Vector3F{ _mm_load_ps(values) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3F FE_VECTORCALL AxisX(const float length = 1.0f)
        {
            return Vector3F{ _mm_blend_ps(_mm_setzero_ps(), _mm_set_ss(length), 0b111) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3F FE_VECTORCALL AxisY(const float length = 1.0f)
        {
            return Vector3F{ _mm_insert_ps(_mm_set_ss(length), _mm_set_ss(length), 0x1d) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3F FE_VECTORCALL AxisZ(const float length = 1.0f)
        {
            return Vector3F{ _mm_insert_ps(_mm_set_ss(length), _mm_set_ss(length), 0x2b) };
        }
    };


    struct PackedVector3F final
    {
        union
        {
            float m_values[3];
            struct
            {
                float x, y, z;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector3F() = default;

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector3F(const float x, const float y, const float z)
            : x(x)
            , y(y)
            , z(z)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector3F(const Vector3F value)
            : x(value.x)
            , y(value.y)
            , z(value.z)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE operator Vector3F() const
        {
            return { x, y, z };
        }
    };

    static_assert(sizeof(PackedVector3F) == 3 * sizeof(float));


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL operator+(const Vector3F lhs, const Vector3F rhs)
    {
        return Vector3F{ _mm_add_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL operator-(const Vector3F lhs, const Vector3F rhs)
    {
        return Vector3F{ _mm_sub_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL operator-(const Vector3F vec)
    {
        const __m128 kSignMask = _mm_castsi128_ps(_mm_setr_epi32(static_cast<int32_t>(0x80000000),
                                                                 static_cast<int32_t>(0x80000000),
                                                                 static_cast<int32_t>(0x80000000),
                                                                 static_cast<int32_t>(0x80000000)));
        return Vector3F{ _mm_xor_ps(vec.m_simdVector, kSignMask) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL operator*(const Vector3F lhs, const Vector3F rhs)
    {
        return Vector3F{ _mm_mul_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL operator/(const Vector3F lhs, const Vector3F rhs)
    {
        return Vector3F{ _mm_div_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL operator*(const Vector3F lhs, const float rhs)
    {
        return Vector3F{ _mm_mul_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL operator/(const Vector3F lhs, const float rhs)
    {
        return Vector3F{ _mm_div_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    namespace Math
    {
        namespace Internal
        {
            FE_FORCE_INLINE FE_NO_SECURITY_COOKIE __m128 FE_VECTORCALL Dot3Impl(const __m128 lhs, const __m128 rhs)
            {
                const __m128 mul = _mm_mul_ps(lhs, rhs);
                const __m128 sum = _mm_add_ps(mul, _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(1, 1, 1, 1)));
                return _mm_add_ps(sum, _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(2, 2, 2, 2)));
            }
        } // namespace Internal


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Dot(const Vector3F lhs, const Vector3F rhs)
        {
            return _mm_cvtss_f32(Internal::Dot3Impl(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Cross(const Vector3F lhs, const Vector3F rhs)
        {
            const __m128 lyzx = _mm_shuffle_ps(lhs.m_simdVector, lhs.m_simdVector, _MM_SHUFFLE(3, 0, 2, 1));
            const __m128 ryzx = _mm_shuffle_ps(rhs.m_simdVector, rhs.m_simdVector, _MM_SHUFFLE(3, 0, 2, 1));
            const __m128 res = _mm_sub_ps(_mm_mul_ps(lhs.m_simdVector, ryzx), _mm_mul_ps(lyzx, rhs.m_simdVector));
            return Vector3F{ _mm_shuffle_ps(res, res, _MM_SHUFFLE(3, 0, 2, 1)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Abs(const Vector3F lhs)
        {
            const __m128 kSignMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
            return Vector3F{ _mm_and_ps(lhs.m_simdVector, kSignMask) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Min<Vector3F>(const Vector3F lhs, const Vector3F rhs)
        {
            return Vector3F{ _mm_min_ps(lhs.m_simdVector, rhs.m_simdVector) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Max<Vector3F>(const Vector3F lhs, const Vector3F rhs)
        {
            return Vector3F{ _mm_max_ps(lhs.m_simdVector, rhs.m_simdVector) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Clamp<Vector3F>(const Vector3F vec, const Vector3F min,
                                                                                     const Vector3F max)
        {
            return Vector3F{ _mm_max_ps(min.m_simdVector, _mm_min_ps(vec.m_simdVector, max.m_simdVector)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Floor(const Vector3F vec)
        {
            return Vector3F{ _mm_floor_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Ceil(const Vector3F vec)
        {
            return Vector3F{ _mm_ceil_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Round(const Vector3F vec)
        {
            return Vector3F{ _mm_floor_ps(_mm_add_ps(vec.m_simdVector, _mm_set_ps1(0.5f))) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Reciprocal(const Vector3F vec)
        {
            return Vector3F{ _mm_div_ps(_mm_set1_ps(1.0f), vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL ReciprocalEstimate(const Vector3F vec)
        {
            return Vector3F{ _mm_rcp_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Sqrt(const Vector3F vec)
        {
            return Vector3F{ _mm_sqrt_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL ReciprocalSqrt(const Vector3F vec)
        {
            return Vector3F{ _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(vec.m_simdVector)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL ReciprocalSqrtEstimate(const Vector3F vec)
        {
            return Vector3F{ _mm_rsqrt_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL LengthSquared(const Vector3F vec)
        {
            return _mm_cvtss_f32(Internal::Dot3Impl(vec.m_simdVector, vec.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Length(const Vector3F vec)
        {
            return _mm_cvtss_f32(_mm_sqrt_ss(Internal::Dot3Impl(vec.m_simdVector, vec.m_simdVector)));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL ReciprocalLength(const Vector3F vec)
        {
            const __m128 lengthSq = Internal::Dot3Impl(vec.m_simdVector, vec.m_simdVector);
            return _mm_cvtss_f32(_mm_div_ss(_mm_set1_ps(1.0f), _mm_sqrt_ss(lengthSq)));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL ReciprocalLengthEstimate(const Vector3F vec)
        {
            const __m128 lengthSq = Internal::Dot3Impl(vec.m_simdVector, vec.m_simdVector);
            return _mm_cvtss_f32(_mm_rsqrt_ss(lengthSq));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Normalize(const Vector3F vec)
        {
            const __m128 lengthSq = Internal::Dot3Impl(vec.m_simdVector, vec.m_simdVector);
            const __m128 broadcast = _mm_shuffle_ps(lengthSq, lengthSq, _MM_SHUFFLE(0, 0, 0, 0));
            return Vector3F{ _mm_div_ps(vec.m_simdVector, _mm_sqrt_ps(broadcast)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL NormalizeEstimate(const Vector3F vec)
        {
            const __m128 lengthSq = Internal::Dot3Impl(vec.m_simdVector, vec.m_simdVector);
            const __m128 broadcast = _mm_shuffle_ps(lengthSq, lengthSq, _MM_SHUFFLE(0, 0, 0, 0));
            return Vector3F{ _mm_mul_ps(vec.m_simdVector, _mm_rsqrt_ps(broadcast)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL DistanceSquared(const Vector3F lhs, const Vector3F rhs)
        {
            return LengthSquared(lhs - rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Distance(const Vector3F lhs, const Vector3F rhs)
        {
            return Length(lhs - rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpEqualMask(const Vector3F lhs, const Vector3F rhs)
        {
            return _mm_movemask_ps(_mm_cmpeq_ps(lhs.m_simdVector, rhs.m_simdVector)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpNotEqualMask(const Vector3F lhs, const Vector3F rhs)
        {
            return _mm_movemask_ps(_mm_cmpneq_ps(lhs.m_simdVector, rhs.m_simdVector)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessMask(const Vector3F lhs, const Vector3F rhs)
        {
            return _mm_movemask_ps(_mm_cmplt_ps(lhs.m_simdVector, rhs.m_simdVector)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterMask(const Vector3F lhs, const Vector3F rhs)
        {
            return _mm_movemask_ps(_mm_cmpgt_ps(lhs.m_simdVector, rhs.m_simdVector)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessEqualMask(const Vector3F lhs, const Vector3F rhs)
        {
            return _mm_movemask_ps(_mm_cmple_ps(lhs.m_simdVector, rhs.m_simdVector)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterEqualMask(const Vector3F lhs, const Vector3F rhs)
        {
            return _mm_movemask_ps(_mm_cmpge_ps(lhs.m_simdVector, rhs.m_simdVector)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EqualEstimate(const Vector3F lhs, const Vector3F rhs,
                                                                               const float epsilon = Constants::kEpsilon)
        {
            const __m128 distance = Abs(lhs - rhs).m_simdVector;
            const uint32_t mask = _mm_movemask_ps(_mm_cmpgt_ps(distance, _mm_set1_ps(epsilon)));
            return (mask & 0b111) == 0;
        }
    } // namespace Math


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator==(const Vector3F lhs, const Vector3F rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) == 0;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator!=(const Vector3F lhs, const Vector3F rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) != 0;
    }
} // namespace FE
