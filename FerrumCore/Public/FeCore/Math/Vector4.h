#pragma once
#include <FeCore/Math/Vector3.h>
#include <FeCore/SIMD/CommonSIMD.h>

namespace FE
{
    //! @brief 4-dimensional vector.
    struct Vector4F final
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

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F(float value)
            : m_simdVector(_mm_set1_ps(value))
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F(__m128 vec)
            : m_simdVector(vec)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F(Vector3F vec, float w)
        {
            const __m128 t = _mm_shuffle_ps(_mm_set_ss(w), vec.m_simdVector, _MM_SHUFFLE(3, 2, 1, 0));
            m_simdVector = _mm_shuffle_ps(vec.m_simdVector, t, _MM_SHUFFLE(0, 2, 1, 0));
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F(float x, float y, float z, float w)
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

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4F FE_VECTORCALL Zero()
        {
            return Vector4F{ _mm_setzero_ps() };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4F FE_VECTORCALL LoadUnaligned(const float* values)
        {
            return Vector4F{ _mm_loadu_ps(values) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4F FE_VECTORCALL LoadAligned(const float* values)
        {
            FE_AssertDebug((reinterpret_cast<uintptr_t>(values) & 15) == 0);
            return Vector4F{ _mm_load_ps(values) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4F FE_VECTORCALL AxisX(float length = 1.0f)
        {
            return Vector4F{ _mm_blend_ps(_mm_setzero_ps(), _mm_set_ss(length), 0b0111) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4F FE_VECTORCALL AxisY(float length = 1.0f)
        {
            return Vector4F{ _mm_insert_ps(_mm_set_ss(length), _mm_set_ss(length), 0x1d) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4F FE_VECTORCALL AxisZ(float length = 1.0f)
        {
            return Vector4F{ _mm_insert_ps(_mm_set_ss(length), _mm_set_ss(length), 0x2b) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4F FE_VECTORCALL AxisW(float length = 1.0f)
        {
            return Vector4F{ _mm_insert_ps(_mm_set_ss(length), _mm_set_ss(length), 0x37) };
        }

        template<Math::Swizzle TSwizzle>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector4F FE_VECTORCALL Swizzle(Vector4F vec)
        {
            return Vector4F{ _mm_shuffle_ps(vec.m_simdVector, vec.m_simdVector, festd::to_underlying(TSwizzle)) };
        }
    };


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL operator+(Vector4F lhs, Vector4F rhs)
    {
        return Vector4F{ _mm_add_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL operator-(Vector4F lhs, Vector4F rhs)
    {
        return Vector4F{ _mm_sub_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL operator-(Vector4F vec)
    {
        const __m128 kSignMask = _mm_castsi128_ps(_mm_setr_epi32(static_cast<int32_t>(0x80000000),
                                                                 static_cast<int32_t>(0x80000000),
                                                                 static_cast<int32_t>(0x80000000),
                                                                 static_cast<int32_t>(0x80000000)));
        return Vector4F{ _mm_xor_ps(vec.m_simdVector, kSignMask) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL operator*(Vector4F lhs, Vector4F rhs)
    {
        return Vector4F{ _mm_mul_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL operator/(Vector4F lhs, Vector4F rhs)
    {
        return Vector4F{ _mm_div_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL operator*(Vector4F lhs, float rhs)
    {
        return Vector4F{ _mm_mul_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL operator/(Vector4F lhs, float rhs)
    {
        return Vector4F{ _mm_div_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    namespace Math
    {
        namespace Internal
        {
            FE_FORCE_INLINE FE_NO_SECURITY_COOKIE __m128 FE_VECTORCALL Dot4Impl(__m128 lhs, __m128 rhs)
            {
                const __m128 mul = _mm_mul_ps(lhs, rhs);
                const __m128 t0 = _mm_shuffle_ps(mul, mul, _MM_SHUFFLE(0, 0, 3, 2));
                const __m128 t1 = _mm_add_ps(mul, t0);
                const __m128 t2 = _mm_shuffle_ps(t1, t1, _MM_SHUFFLE(0, 0, 0, 1));
                return _mm_add_ps(t1, t2);
            }
        } // namespace Internal


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL DotBroadcast(Vector4F lhs, Vector4F rhs)
        {
            const __m128 result = Internal::Dot4Impl(lhs.m_simdVector, rhs.m_simdVector);
            return Vector4F{ _mm_shuffle_ps(result, result, _MM_SHUFFLE(0, 0, 0, 0)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Dot(Vector4F lhs, Vector4F rhs)
        {
            return _mm_cvtss_f32(Internal::Dot4Impl(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL Abs(Vector4F lhs)
        {
            const __m128 kSignMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
            return Vector4F{ _mm_and_ps(lhs.m_simdVector, kSignMask) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL Min<Vector4F>(Vector4F lhs, Vector4F rhs)
        {
            return Vector4F{ _mm_min_ps(lhs.m_simdVector, rhs.m_simdVector) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL Max<Vector4F>(Vector4F lhs, Vector4F rhs)
        {
            return Vector4F{ _mm_max_ps(lhs.m_simdVector, rhs.m_simdVector) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL Clamp<Vector4F>(Vector4F vec, Vector4F min, Vector4F max)
        {
            return Vector4F{ _mm_max_ps(min.m_simdVector, _mm_min_ps(vec.m_simdVector, max.m_simdVector)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL Saturate(Vector4F vec)
        {
            return Vector4F{ _mm_max_ps(_mm_setzero_ps(), _mm_min_ps(vec.m_simdVector, _mm_set1_ps(1.0f))) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL Floor(Vector4F vec)
        {
            return Vector4F{ _mm_floor_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL Ceil(Vector4F vec)
        {
            return Vector4F{ _mm_ceil_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL Round(Vector4F vec)
        {
            return Vector4F{ _mm_floor_ps(_mm_add_ps(vec.m_simdVector, _mm_set_ps1(0.5f))) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL Reciprocal(Vector4F vec)
        {
            return Vector4F{ _mm_div_ps(_mm_set_ps1(1.0f), vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL ReciprocalEstimate(Vector4F vec)
        {
            return Vector4F{ _mm_rcp_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL Sqrt(Vector4F vec)
        {
            return Vector4F{ _mm_sqrt_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL ReciprocalSqrt(Vector4F vec)
        {
            return Vector4F{ _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(vec.m_simdVector)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL ReciprocalSqrtEstimate(Vector4F vec)
        {
            return Vector4F{ _mm_rsqrt_ps(vec.m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL LengthSquaredBroadcast(Vector4F vec)
        {
            return DotBroadcast(vec, vec);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL LengthSquared(Vector4F vec)
        {
            return _mm_cvtss_f32(Internal::Dot4Impl(vec.m_simdVector, vec.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL LengthBroadcast(Vector4F vec)
        {
            return Vector4F{ _mm_sqrt_ps(DotBroadcast(vec, vec).m_simdVector) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Length(Vector4F vec)
        {
            return _mm_cvtss_f32(_mm_sqrt_ss(Internal::Dot4Impl(vec.m_simdVector, vec.m_simdVector)));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL ReciprocalLength(Vector4F vec)
        {
            const __m128 lengthSq = Internal::Dot4Impl(vec.m_simdVector, vec.m_simdVector);
            return _mm_cvtss_f32(_mm_div_ss(_mm_set1_ps(1.0f), _mm_sqrt_ss(lengthSq)));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL ReciprocalLengthEstimate(Vector4F vec)
        {
            const __m128 lengthSq = Internal::Dot4Impl(vec.m_simdVector, vec.m_simdVector);
            return _mm_cvtss_f32(_mm_rsqrt_ss(lengthSq));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL Normalize(Vector4F vec)
        {
            const __m128 lengthSq = Internal::Dot4Impl(vec.m_simdVector, vec.m_simdVector);
            const __m128 broadcast = _mm_shuffle_ps(lengthSq, lengthSq, _MM_SHUFFLE(0, 0, 0, 0));
            return Vector4F{ _mm_div_ps(vec.m_simdVector, _mm_sqrt_ps(broadcast)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL NormalizeEstimate(Vector4F vec)
        {
            const __m128 lengthSq = Internal::Dot4Impl(vec.m_simdVector, vec.m_simdVector);
            const __m128 broadcast = _mm_shuffle_ps(lengthSq, lengthSq, _MM_SHUFFLE(0, 0, 0, 0));
            return Vector4F{ _mm_mul_ps(vec.m_simdVector, _mm_rsqrt_ps(broadcast)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL DistanceSquared(Vector4F lhs, Vector4F rhs)
        {
            return LengthSquared(lhs - rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Distance(Vector4F lhs, Vector4F rhs)
        {
            return Length(lhs - rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpEqualMask(Vector4F lhs, Vector4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpeq_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpNotEqualMask(Vector4F lhs, Vector4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpneq_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessMask(Vector4F lhs, Vector4F rhs)
        {
            return _mm_movemask_ps(_mm_cmplt_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterMask(Vector4F lhs, Vector4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpgt_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessEqualMask(Vector4F lhs, Vector4F rhs)
        {
            return _mm_movemask_ps(_mm_cmple_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterEqualMask(Vector4F lhs, Vector4F rhs)
        {
            return _mm_movemask_ps(_mm_cmpge_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EqualEstimate(Vector4F lhs, Vector4F rhs,
                                                                               float epsilon = Constants::Epsilon)
        {
            const __m128 distance = Abs(lhs - rhs).m_simdVector;
            const uint32_t mask = _mm_movemask_ps(_mm_cmpgt_ps(distance, _mm_set1_ps(epsilon)));
            return mask == 0;
        }
    } // namespace Math


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator==(Vector4F lhs, Vector4F rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) == 0;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator!=(Vector4F lhs, Vector4F rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) != 0;
    }
} // namespace FE
