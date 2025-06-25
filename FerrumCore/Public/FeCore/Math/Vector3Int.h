#pragma once
#include <FeCore/Math/Vector3.h>

namespace FE
{
    struct Vector3Int final
    {
        union
        {
            __m128i m_simdVector;
            int32_t m_values[3];
            struct
            {
                int32_t x, y, z;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int(ForceInitType)
            : m_simdVector(_mm_setzero_si128())
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int(const int32_t value)
            : m_simdVector(_mm_set1_epi32(value))
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int(const __m128i vec)
            : m_simdVector(vec)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int(const Vector3 vec)
            : m_simdVector(_mm_cvtps_epi32(vec.m_simdVector))
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int(const int32_t x, const int32_t y, const int32_t z)
            : m_simdVector(_mm_setr_epi32(x, y, z, 0))
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE int32_t* FE_VECTORCALL Data()
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE const int32_t* FE_VECTORCALL Data() const
        {
            return m_values;
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE operator Vector3() const
        {
            return Vector3{ _mm_cvtepi32_ps(m_simdVector) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3Int FE_VECTORCALL Zero()
        {
            return Vector3Int{ _mm_setzero_si128() };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3Int FE_VECTORCALL LoadUnaligned(const int32_t* values)
        {
            return Vector3Int{ _mm_loadu_si128(reinterpret_cast<const __m128i*>(values)) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3Int FE_VECTORCALL LoadAligned(const int32_t* values)
        {
            FE_AssertDebug((reinterpret_cast<uintptr_t>(values) & 15) == 0);
            return Vector3Int{ _mm_load_si128(reinterpret_cast<const __m128i*>(values)) };
        }
    };


    struct PackedVector3Int final
    {
        union
        {
            int32_t m_values[3];
            struct
            {
                int32_t x, y, z;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector3Int() = default;

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector3Int(const int32_t x, const int32_t y, const int32_t z)
            : x(x)
            , y(y)
            , z(z)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector3Int(const Vector3Int value)
            : x(value.x)
            , y(value.y)
            , z(value.z)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE operator Vector3Int() const
        {
            return { x, y, z };
        }
    };

    static_assert(sizeof(PackedVector3Int) == 3 * sizeof(int32_t));


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int FE_VECTORCALL operator+(const Vector3Int lhs, const Vector3Int rhs)
    {
        return Vector3Int{ _mm_add_epi32(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int FE_VECTORCALL operator-(const Vector3Int lhs, const Vector3Int rhs)
    {
        return Vector3Int{ _mm_sub_epi32(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int FE_VECTORCALL operator-(const Vector3Int vec)
    {
        const __m128i flipped = _mm_xor_si128(vec.m_simdVector, _mm_set1_epi32(static_cast<int32_t>(UINT32_MAX)));
        return Vector3Int{ _mm_add_epi32(flipped, _mm_set1_epi32(1)) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int FE_VECTORCALL operator*(const Vector3Int lhs, const Vector3Int rhs)
    {
        return Vector3Int{ _mm_mul_epi32(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int FE_VECTORCALL operator*(const Vector3Int lhs, const int32_t rhs)
    {
        return Vector3Int{ _mm_mul_epi32(lhs.m_simdVector, _mm_set1_epi32(rhs)) };
    }


    namespace Math
    {
        namespace Internal
        {
            FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL MoveMask(const __m128i mask)
            {
                return _mm_movemask_ps(_mm_castsi128_ps(mask));
            }


            FE_FORCE_INLINE FE_NO_SECURITY_COOKIE __m128i FE_VECTORCALL FlipMask(const __m128i mask)
            {
                return _mm_xor_si128(mask, _mm_set1_epi32(static_cast<int32_t>(Constants::kMaxU32)));
            }
        } // namespace Internal


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int FE_VECTORCALL Min<Vector3Int>(const Vector3Int lhs, const Vector3Int rhs)
        {
            return Vector3Int{ _mm_min_epi32(lhs.m_simdVector, rhs.m_simdVector) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int FE_VECTORCALL Max<Vector3Int>(const Vector3Int lhs, const Vector3Int rhs)
        {
            return Vector3Int{ _mm_max_epi32(lhs.m_simdVector, rhs.m_simdVector) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3Int FE_VECTORCALL Clamp<Vector3Int>(const Vector3Int vec,
                                                                                         const Vector3Int min,
                                                                                         const Vector3Int max)
        {
            return Vector3Int{ _mm_max_epi32(min.m_simdVector, _mm_min_epi32(vec.m_simdVector, max.m_simdVector)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpEqualMask(const Vector3Int lhs, const Vector3Int rhs)
        {
            return Internal::MoveMask(_mm_cmpeq_epi32(lhs.m_simdVector, rhs.m_simdVector)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpNotEqualMask(const Vector3Int lhs, const Vector3Int rhs)
        {
            const __m128i eqMask = _mm_cmpeq_epi32(lhs.m_simdVector, rhs.m_simdVector);
            return Internal::MoveMask(Internal::FlipMask(eqMask)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessMask(const Vector3Int lhs, const Vector3Int rhs)
        {
            return Internal::MoveMask(_mm_cmplt_epi32(lhs.m_simdVector, rhs.m_simdVector)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterMask(const Vector3Int lhs, const Vector3Int rhs)
        {
            return Internal::MoveMask(_mm_cmpgt_epi32(lhs.m_simdVector, rhs.m_simdVector)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessEqualMask(const Vector3Int lhs, const Vector3Int rhs)
        {
            const __m128i gtMask = _mm_cmpgt_epi32(lhs.m_simdVector, rhs.m_simdVector);
            return Internal::MoveMask(Internal::FlipMask(gtMask)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterEqualMask(const Vector3Int lhs,
                                                                                         const Vector3Int rhs)
        {
            const __m128i ltMask = _mm_cmplt_epi32(lhs.m_simdVector, rhs.m_simdVector);
            return Internal::MoveMask(Internal::FlipMask(ltMask)) & 0b111;
        }
    } // namespace Math


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator==(const Vector3Int lhs, const Vector3Int rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) == 0;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator!=(const Vector3Int lhs, const Vector3Int rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) != 0;
    }
} // namespace FE
