#pragma once
#include <FeCore/Math/Vector3.h>
#include <FeCore/SIMD/Utils.h>

namespace FE
{
    //! @brief 4-dimensional vector.
    struct Vector4 final
    {
        union
        {
            __m128 m_simdVector;
            float m_values[4];

            struct
            {
                float x, y, z, w;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4(ForceInitType)
            : m_simdVector(_mm_setzero_ps())
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4(const float value)
            : m_simdVector(_mm_set1_ps(value))
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4(const __m128 vec)
            : m_simdVector(vec)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4(const Vector3 vec, const float w)
        {
            const __m128 t = _mm_shuffle_ps(_mm_set_ss(w), vec.m_simdVector, _MM_SHUFFLE(3, 2, 1, 0));
            m_simdVector = _mm_shuffle_ps(vec.m_simdVector, t, _MM_SHUFFLE(0, 2, 1, 0));
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4(const float x, const float y, const float z, const float w)
            : m_simdVector(_mm_setr_ps(x, y, z, w))
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

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float operator[](const uint32_t index) const
        {
            return m_values[index];
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL Zero()
        {
            return Vector4{ _mm_setzero_ps() };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL LoadUnaligned(const float* values)
        {
            return Vector4{ _mm_loadu_ps(values) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL LoadAligned(const float* values)
        {
            FE_AssertDebug((reinterpret_cast<uintptr_t>(values) & 15) == 0);
            return Vector4{ _mm_load_ps(values) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL LoadUnaligned(const double* values)
        {
            const __m256d doubles = _mm256_loadu_pd(values);
            return Vector4{ _mm256_cvtpd_ps(doubles) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL LoadAligned(const double* values)
        {
            FE_AssertDebug((reinterpret_cast<uintptr_t>(values) & 31) == 0);
            const __m256d doubles = _mm256_load_pd(values);
            return Vector4{ _mm256_cvtpd_ps(doubles) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL AxisX(const float length = 1.0f)
        {
            return Vector4{ _mm_blend_ps(_mm_setzero_ps(), _mm_set_ss(length), 0b0111) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL AxisY(const float length = 1.0f)
        {
            return Vector4{ _mm_insert_ps(_mm_set_ss(length), _mm_set_ss(length), 0x1d) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL AxisZ(const float length = 1.0f)
        {
            return Vector4{ _mm_insert_ps(_mm_set_ss(length), _mm_set_ss(length), 0x2b) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL AxisW(const float length = 1.0f)
        {
            return Vector4{ _mm_insert_ps(_mm_set_ss(length), _mm_set_ss(length), 0x37) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL WorldForward()
        {
            return Vector4{ 0.0f, 0.0f, 1.0f, 0.0f };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL WorldUp()
        {
            return Vector4{ 0.0f, 1.0f, 0.0f, 0.0f };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL WorldRight()
        {
            return Vector4{ 1.0f, 0.0f, 0.0f, 0.0f };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL BroadcastX(const Vector4 vec)
        {
            return Vector4{ _mm_shuffle_ps(vec.m_simdVector, vec.m_simdVector, _MM_SHUFFLE(0, 0, 0, 0)) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL BroadcastY(const Vector4 vec)
        {
            return Vector4{ _mm_shuffle_ps(vec.m_simdVector, vec.m_simdVector, _MM_SHUFFLE(1, 1, 1, 1)) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL BroadcastZ(const Vector4 vec)
        {
            return Vector4{ _mm_shuffle_ps(vec.m_simdVector, vec.m_simdVector, _MM_SHUFFLE(2, 2, 2, 2)) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4 FE_VECTORCALL BroadcastW(const Vector4 vec)
        {
            return Vector4{ _mm_shuffle_ps(vec.m_simdVector, vec.m_simdVector, _MM_SHUFFLE(3, 3, 3, 3)) };
        }
    };


    struct PackedVector4F final
    {
        union
        {
            float m_values[4];
            struct
            {
                float x, y, z, w;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector4F() = default;

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector4F(const float x, const float y, const float z, const float w)
            : x(x)
            , y(y)
            , z(z)
            , w(w)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector4F(const Vector4 value)
            : x(value.x)
            , y(value.y)
            , z(value.z)
            , w(value.w)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE operator Vector4() const
        {
            return { x, y, z, w };
        }
    };

    static_assert(sizeof(PackedVector4F) == 4 * sizeof(float));


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL operator+(const Vector4 lhs, const Vector4 rhs)
    {
        return Vector4{ _mm_add_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4& FE_VECTORCALL operator+=(Vector4& lhs, const Vector4 rhs)
    {
        return lhs = lhs + rhs;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL operator-(const Vector4 lhs, const Vector4 rhs)
    {
        return Vector4{ _mm_sub_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4& FE_VECTORCALL operator-=(Vector4& lhs, const Vector4 rhs)
    {
        return lhs = lhs - rhs;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL operator-(const Vector4 vec)
    {
        const __m128 kSignMask = _mm_castsi128_ps(_mm_setr_epi32(static_cast<int32_t>(0x80000000),
                                                                 static_cast<int32_t>(0x80000000),
                                                                 static_cast<int32_t>(0x80000000),
                                                                 static_cast<int32_t>(0x80000000)));
        return Vector4{ _mm_xor_ps(vec.m_simdVector, kSignMask) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL operator*(const Vector4 lhs, const Vector4 rhs)
    {
        return Vector4{ _mm_mul_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4& FE_VECTORCALL operator*=(Vector4& lhs, const Vector4 rhs)
    {
        return lhs = lhs * rhs;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL operator/(const Vector4 lhs, const Vector4 rhs)
    {
        return Vector4{ _mm_div_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4& FE_VECTORCALL operator/=(Vector4& lhs, const Vector4 rhs)
    {
        return lhs = lhs / rhs;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL operator*(const Vector4 lhs, const float rhs)
    {
        return Vector4{ _mm_mul_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4& FE_VECTORCALL operator*=(Vector4& lhs, const float rhs)
    {
        return lhs = lhs * rhs;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL operator/(const Vector4 lhs, const float rhs)
    {
        return Vector4{ _mm_div_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4& FE_VECTORCALL operator/=(Vector4& lhs, const float rhs)
    {
        return lhs = lhs / rhs;
    }


    namespace Math
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL DotBroadcast(const Vector4 lhs, const Vector4 rhs)
        {
            const __m128 result = SIMD::DotProduct(lhs.m_simdVector, rhs.m_simdVector);
            return Vector4{ _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 0, 0, 0)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Dot(const Vector4 lhs, const Vector4 rhs)
        {
            return _mm_cvtss_f32(SIMD::DotProduct(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL Abs(const Vector4 lhs)
        {
            const __m128 kSignMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
            return Vector4{ _mm_and_ps(lhs.m_simdVector, kSignMask) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL Min<Vector4>(const Vector4 lhs, const Vector4 rhs)
        {
            return Vector4{ _mm_min_ps(lhs.m_simdVector, rhs.m_simdVector) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL Max<Vector4>(const Vector4 lhs, const Vector4 rhs)
        {
            return Vector4{ _mm_max_ps(lhs.m_simdVector, rhs.m_simdVector) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL Clamp<Vector4>(const Vector4 vec, const Vector4 min,
                                                                                   const Vector4 max)
        {
            return Vector4{ _mm_max_ps(min.m_simdVector, _mm_min_ps(vec.m_simdVector, max.m_simdVector)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL Saturate(const Vector4 vec)
        {
            return Vector4{ _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(vec.m_simdVector, _mm_set1_ps(1.0f))) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL Floor(const Vector4 vec)
        {
            return Vector4{ _mm_floor_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL Ceil(const Vector4 vec)
        {
            return Vector4{ _mm_ceil_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL Round(const Vector4 vec)
        {
            return Vector4{ _mm_floor_ps(_mm_add_ps(vec.m_simdVector, _mm_set_ps1(0.5f))) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL Reciprocal(const Vector4 vec)
        {
            return Vector4{ _mm_div_ps(_mm_set_ps1(1.0f), vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL ReciprocalEstimate(const Vector4 vec)
        {
            return Vector4{ _mm_rcp_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL Sqrt(const Vector4 vec)
        {
            return Vector4{ _mm_sqrt_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL ReciprocalSqrt(const Vector4 vec)
        {
            return Vector4{ _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(vec.m_simdVector)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL ReciprocalSqrtEstimate(const Vector4 vec)
        {
            return Vector4{ _mm_rsqrt_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL LengthSquaredBroadcast(const Vector4 vec)
        {
            return DotBroadcast(vec, vec);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL LengthSquared(const Vector4 vec)
        {
            return _mm_cvtss_f32(SIMD::DotProduct(vec.m_simdVector, vec.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL LengthBroadcast(const Vector4 vec)
        {
            return Vector4{ _mm_sqrt_ps(DotBroadcast(vec, vec).m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Length(const Vector4 vec)
        {
            return _mm_cvtss_f32(_mm_sqrt_ss(SIMD::DotProduct(vec.m_simdVector, vec.m_simdVector)));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL ReciprocalLength(const Vector4 vec)
        {
            const __m128 lengthSq = SIMD::DotProduct(vec.m_simdVector, vec.m_simdVector);
            return _mm_cvtss_f32(_mm_div_ss(_mm_set1_ps(1.0f), _mm_sqrt_ss(lengthSq)));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL ReciprocalLengthEstimate(const Vector4 vec)
        {
            const __m128 lengthSq = SIMD::DotProduct(vec.m_simdVector, vec.m_simdVector);
            return _mm_cvtss_f32(_mm_rsqrt_ss(lengthSq));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL Normalize(const Vector4 vec)
        {
            const __m128 lengthSq = SIMD::DotProduct(vec.m_simdVector, vec.m_simdVector);
            const __m128 broadcast = _mm_shuffle_ps(lengthSq, lengthSq, _MM_SHUFFLE(0, 0, 0, 0));
            return Vector4{ _mm_div_ps(vec.m_simdVector, _mm_sqrt_ps(broadcast)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL NormalizeEstimate(const Vector4 vec)
        {
            const __m128 lengthSq = SIMD::DotProduct(vec.m_simdVector, vec.m_simdVector);
            const __m128 broadcast = _mm_shuffle_ps(lengthSq, lengthSq, _MM_SHUFFLE(0, 0, 0, 0));
            return Vector4{ _mm_mul_ps(vec.m_simdVector, _mm_rsqrt_ps(broadcast)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL DistanceSquared(const Vector4 lhs, const Vector4 rhs)
        {
            return LengthSquared(lhs - rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Distance(const Vector4 lhs, const Vector4 rhs)
        {
            return Length(lhs - rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpEqualMask(const Vector4 lhs, const Vector4 rhs)
        {
            return _mm_movemask_ps(_mm_cmpeq_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpNotEqualMask(const Vector4 lhs, const Vector4 rhs)
        {
            return _mm_movemask_ps(_mm_cmpneq_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessMask(const Vector4 lhs, const Vector4 rhs)
        {
            return _mm_movemask_ps(_mm_cmplt_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterMask(const Vector4 lhs, const Vector4 rhs)
        {
            return _mm_movemask_ps(_mm_cmpgt_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessEqualMask(const Vector4 lhs, const Vector4 rhs)
        {
            return _mm_movemask_ps(_mm_cmple_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterEqualMask(const Vector4 lhs, const Vector4 rhs)
        {
            return _mm_movemask_ps(_mm_cmpge_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EqualEstimate(const Vector4 lhs, const Vector4 rhs,
                                                                               const float epsilon = Constants::kEpsilon)
        {
            const __m128 distance = Abs(lhs - rhs).m_simdVector;
            const uint32_t mask = _mm_movemask_ps(_mm_cmpgt_ps(distance, _mm_set1_ps(epsilon)));
            return mask == 0;
        }
    } // namespace Math


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator==(const Vector4 lhs, const Vector4 rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) == 0;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator!=(const Vector4 lhs, const Vector4 rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) != 0;
    }
} // namespace FE
