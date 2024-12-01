#pragma once
#include <FeCore/Math/Vector4.h>

namespace FE
{
    struct Quaternion final
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

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion(__m128 vec)
            : m_simdVector(vec)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion(Vector3F vec, float w)
        {
            const __m128 t = _mm_shuffle_ps(_mm_set_ss(w), vec.m_simdVector, _MM_SHUFFLE(3, 2, 1, 0));
            m_simdVector = _mm_shuffle_ps(vec.m_simdVector, t, _MM_SHUFFLE(0, 2, 1, 0));
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion(float x, float y, float z, float w)
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

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Quaternion FE_VECTORCALL Zero()
        {
            return Quaternion{ _mm_setzero_ps() };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Quaternion FE_VECTORCALL Identity()
        {
            return Quaternion{ 0.0f, 0.0f, 0.0f, 1.0f };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Quaternion FE_VECTORCALL LoadUnaligned(const float* values)
        {
            return Quaternion{ _mm_loadu_ps(values) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Quaternion FE_VECTORCALL LoadAligned(const float* values)
        {
            FE_AssertDebug((reinterpret_cast<uintptr_t>(values) & 15) == 0);
            return Quaternion{ _mm_load_ps(values) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Quaternion FE_VECTORCALL RotationX(float angle)
        {
            const float sin = Math::Sin(angle * 0.5f);
            const float cos = Math::Cos(angle * 0.5f);
            return Quaternion{ sin, 0.0f, 0.0f, cos };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Quaternion FE_VECTORCALL RotationY(float angle)
        {
            const float sin = Math::Sin(angle * 0.5f);
            const float cos = Math::Cos(angle * 0.5f);
            return Quaternion{ 0.0f, sin, 0.0f, cos };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Quaternion FE_VECTORCALL RotationZ(float angle)
        {
            const float sin = Math::Sin(angle * 0.5f);
            const float cos = Math::Cos(angle * 0.5f);
            return Quaternion{ 0.0f, 0.0f, sin, cos };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Quaternion FE_VECTORCALL AxisAngle(Vector3F axis, float angle)
        {
            const float sin = Math::Sin(angle * 0.5f);
            const float cos = Math::Cos(angle * 0.5f);
            return Quaternion{ axis * sin, cos };
        }
    };


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion FE_VECTORCALL operator+(Quaternion lhs, Quaternion rhs)
    {
        return Quaternion{ _mm_add_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion FE_VECTORCALL operator-(Quaternion lhs, Quaternion rhs)
    {
        return Quaternion{ _mm_sub_ps(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion FE_VECTORCALL operator*(Quaternion lhs, float rhs)
    {
        return Quaternion{ _mm_mul_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion FE_VECTORCALL operator/(Quaternion lhs, float rhs)
    {
        return Quaternion{ _mm_div_ps(lhs.m_simdVector, _mm_set1_ps(rhs)) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion FE_VECTORCALL operator*(Quaternion lhs, Quaternion rhs)
    {
        const __m128 a1123 = _mm_shuffle_ps(lhs.m_simdVector, lhs.m_simdVector, _MM_SHUFFLE(3, 2, 1, 1));
        const __m128 a2231 = _mm_shuffle_ps(lhs.m_simdVector, lhs.m_simdVector, _MM_SHUFFLE(1, 3, 2, 2));
        const __m128 b1000 = _mm_shuffle_ps(rhs.m_simdVector, rhs.m_simdVector, _MM_SHUFFLE(0, 0, 0, 1));
        const __m128 b2312 = _mm_shuffle_ps(rhs.m_simdVector, rhs.m_simdVector, _MM_SHUFFLE(2, 1, 3, 2));

        const __m128 t1 = _mm_mul_ps(a1123, b1000);
        const __m128 t2 = _mm_mul_ps(a2231, b2312);

        const __m128 kNegateWMask = _mm_castsi128_ps(_mm_setr_epi32(0, 0, 0, static_cast<int32_t>(0x80000000)));
        const __m128 t12 = _mm_xor_ps(_mm_mul_ps(t1, t2), kNegateWMask);

        const __m128 a3312 = _mm_shuffle_ps(lhs.m_simdVector, lhs.m_simdVector, _MM_SHUFFLE(2, 1, 3, 3));
        const __m128 b3231 = _mm_shuffle_ps(rhs.m_simdVector, rhs.m_simdVector, _MM_SHUFFLE(1, 3, 2, 3));
        const __m128 a0000 = _mm_shuffle_ps(lhs.m_simdVector, lhs.m_simdVector, _MM_SHUFFLE(0, 0, 0, 0));

        const __m128 t3 = _mm_mul_ps(a3312, b3231);
        const __m128 t0 = _mm_mul_ps(a0000, rhs.m_simdVector);
        const __m128 t03 = _mm_sub_ps(t0, t3);
        return Quaternion{ _mm_add_ps(t03, t12) };
    }


    namespace Math
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Im(Quaternion quat)
        {
            return Vector3F{ quat.m_simdVector };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Re(Quaternion quat)
        {
            return quat.w;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Dot(Quaternion lhs, Quaternion rhs)
        {
            return _mm_cvtss_f32(Internal::Dot4Impl(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL LengthSquared(Quaternion quat)
        {
            return _mm_cvtss_f32(Internal::Dot4Impl(quat.m_simdVector, quat.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Length(Quaternion quat)
        {
            return _mm_cvtss_f32(_mm_sqrt_ss(Internal::Dot4Impl(quat.m_simdVector, quat.m_simdVector)));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL ReciprocalLength(Quaternion quat)
        {
            const __m128 lengthSq = Internal::Dot4Impl(quat.m_simdVector, quat.m_simdVector);
            return _mm_cvtss_f32(_mm_div_ss(_mm_set1_ps(1.0f), _mm_sqrt_ss(lengthSq)));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL ReciprocalLengthEstimate(Quaternion quat)
        {
            const __m128 lengthSq = Internal::Dot4Impl(quat.m_simdVector, quat.m_simdVector);
            return _mm_cvtss_f32(_mm_rsqrt_ss(lengthSq));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion FE_VECTORCALL Normalize(Quaternion quat)
        {
            const __m128 lengthSq = Internal::Dot4Impl(quat.m_simdVector, quat.m_simdVector);
            const __m128 broadcast = _mm_shuffle_ps(lengthSq, lengthSq, _MM_SHUFFLE(0, 0, 0, 0));
            return Quaternion{ _mm_div_ps(quat.m_simdVector, _mm_sqrt_ps(broadcast)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion FE_VECTORCALL NormalizeEstimate(Quaternion quat)
        {
            const __m128 lengthSq = Internal::Dot4Impl(quat.m_simdVector, quat.m_simdVector);
            const __m128 broadcast = _mm_shuffle_ps(lengthSq, lengthSq, _MM_SHUFFLE(0, 0, 0, 0));
            return Quaternion{ _mm_mul_ps(quat.m_simdVector, _mm_rsqrt_ps(broadcast)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpEqualMask(Quaternion lhs, Quaternion rhs)
        {
            return _mm_movemask_ps(_mm_cmpeq_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpNotEqualMask(Quaternion lhs, Quaternion rhs)
        {
            return _mm_movemask_ps(_mm_cmpneq_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessMask(Quaternion lhs, Quaternion rhs)
        {
            return _mm_movemask_ps(_mm_cmplt_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterMask(Quaternion lhs, Quaternion rhs)
        {
            return _mm_movemask_ps(_mm_cmpgt_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessEqualMask(Quaternion lhs, Quaternion rhs)
        {
            return _mm_movemask_ps(_mm_cmple_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterEqualMask(Quaternion lhs, Quaternion rhs)
        {
            return _mm_movemask_ps(_mm_cmpge_ps(lhs.m_simdVector, rhs.m_simdVector));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EqualEstimate(Quaternion lhs, Quaternion rhs,
                                                                               float epsilon = Constants::Epsilon)
        {
            const __m128 kSignMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
            const __m128 distance = _mm_and_ps(_mm_sub_ps(lhs.m_simdVector, rhs.m_simdVector), kSignMask);
            const uint32_t mask = _mm_movemask_ps(_mm_cmpgt_ps(distance, _mm_set1_ps(epsilon)));
            return mask == 0;
        }
    } // namespace Math


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator==(Quaternion lhs, Quaternion rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) == 0;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator!=(Quaternion lhs, Quaternion rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) != 0;
    }
} // namespace FE
