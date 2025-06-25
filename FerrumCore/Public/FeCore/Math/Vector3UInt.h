#pragma once
#include <FeCore/Math/Vector3Int.h>

namespace FE
{
    struct Vector3UInt final
    {
        union
        {
            __m128i m_simdVector;
            uint32_t m_values[3];
            struct
            {
                uint32_t x, y, z;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt(ForceInitType)
            : m_simdVector(_mm_setzero_si128())
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt(const uint32_t value)
            : m_simdVector(_mm_set1_epi32(static_cast<int32_t>(value)))
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt(const __m128i vec)
            : m_simdVector(vec)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt(const Vector3 vec)
            : m_simdVector(_mm_cvtps_epi32(vec.m_simdVector))
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt(const Vector3Int vec)
            : m_simdVector(vec.m_simdVector)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt(const uint32_t x, const uint32_t y, const uint32_t z)
            : m_simdVector(_mm_setr_epi32(static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(z), 0))
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t* FE_VECTORCALL Data()
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE const uint32_t* FE_VECTORCALL Data() const
        {
            return m_values;
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE operator Vector3() const
        {
            const __m128i lo = _mm_and_si128(m_simdVector, _mm_set1_epi32(0xffff));
            const __m128i hi = _mm_srli_epi32(m_simdVector, 16);
            const __m128 loFloat = _mm_cvtepi32_ps(lo);
            const __m128 hiFloat = _mm_mul_ps(_mm_set1_ps(1 << 16), _mm_cvtepi32_ps(hi));
            return Vector3{ _mm_add_ps(hiFloat, loFloat) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3UInt FE_VECTORCALL Zero()
        {
            return Vector3UInt{ _mm_setzero_si128() };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3UInt FE_VECTORCALL LoadUnaligned(const int32_t* values)
        {
            return Vector3UInt{ _mm_loadu_si128(reinterpret_cast<const __m128i*>(values)) };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector3UInt FE_VECTORCALL LoadAligned(const int32_t* values)
        {
            FE_AssertDebug((reinterpret_cast<uintptr_t>(values) & 15) == 0);
            return Vector3UInt{ _mm_load_si128(reinterpret_cast<const __m128i*>(values)) };
        }
    };


    struct PackedVector3UInt final
    {
        union
        {
            uint32_t m_values[3];
            struct
            {
                uint32_t x, y, z;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector3UInt() = default;

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector3UInt(const uint32_t x, const uint32_t y, const uint32_t z)
            : x(x)
            , y(y)
            , z(z)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedVector3UInt(const Vector3UInt value)
            : x(value.x)
            , y(value.y)
            , z(value.z)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE operator Vector3UInt() const
        {
            return { x, y, z };
        }
    };

    static_assert(sizeof(PackedVector3UInt) == 3 * sizeof(uint32_t));


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt FE_VECTORCALL operator+(const Vector3UInt lhs, const Vector3UInt rhs)
    {
        return Vector3UInt{ _mm_add_epi32(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt FE_VECTORCALL operator-(const Vector3UInt lhs, const Vector3UInt rhs)
    {
        return Vector3UInt{ _mm_sub_epi32(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt FE_VECTORCALL operator*(const Vector3UInt lhs, const Vector3UInt rhs)
    {
        return Vector3UInt{ _mm_mul_epu32(lhs.m_simdVector, rhs.m_simdVector) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt FE_VECTORCALL operator*(const Vector3UInt lhs, const uint32_t rhs)
    {
        return Vector3UInt{ _mm_mul_epu32(lhs.m_simdVector, _mm_set1_epi32(static_cast<int32_t>(rhs))) };
    }


    namespace Math
    {
        namespace Internal
        {
            FE_FORCE_INLINE FE_NO_SECURITY_COOKIE __m128i FE_VECTORCALL RemapUnsigned(const __m128i value)
            {
                return _mm_xor_si128(value, _mm_set1_epi32(Constants::kMinI32));
            }
        } // namespace Internal


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt FE_VECTORCALL Min<Vector3UInt>(const Vector3UInt lhs,
                                                                                         const Vector3UInt rhs)
        {
            return Vector3UInt{ _mm_min_epu32(lhs.m_simdVector, rhs.m_simdVector) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt FE_VECTORCALL Max<Vector3UInt>(const Vector3UInt lhs,
                                                                                         const Vector3UInt rhs)
        {
            return Vector3UInt{ _mm_max_epu32(lhs.m_simdVector, rhs.m_simdVector) };
        }


        template<>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3UInt FE_VECTORCALL Clamp<Vector3UInt>(const Vector3UInt vec,
                                                                                           const Vector3UInt min,
                                                                                           const Vector3UInt max)
        {
            return Vector3UInt{ _mm_max_epu32(min.m_simdVector, _mm_min_epu32(vec.m_simdVector, max.m_simdVector)) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpEqualMask(const Vector3UInt lhs, const Vector3UInt rhs)
        {
            return Internal::MoveMask(_mm_cmpeq_epi32(lhs.m_simdVector, rhs.m_simdVector)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpNotEqualMask(const Vector3UInt lhs, const Vector3UInt rhs)
        {
            const __m128i eqMask = _mm_cmpeq_epi32(lhs.m_simdVector, rhs.m_simdVector);
            return Internal::MoveMask(Internal::FlipMask(eqMask)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessMask(const Vector3UInt lhs, const Vector3UInt rhs)
        {
            const __m128i lhsSigned = Internal::RemapUnsigned(lhs.m_simdVector);
            const __m128i rhsSigned = Internal::RemapUnsigned(rhs.m_simdVector);
            return Internal::MoveMask(_mm_cmplt_epi32(lhsSigned, rhsSigned)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterMask(const Vector3UInt lhs, const Vector3UInt rhs)
        {
            const __m128i lhsSigned = Internal::RemapUnsigned(lhs.m_simdVector);
            const __m128i rhsSigned = Internal::RemapUnsigned(rhs.m_simdVector);
            return Internal::MoveMask(_mm_cmpgt_epi32(lhsSigned, rhsSigned)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpLessEqualMask(const Vector3UInt lhs,
                                                                                      const Vector3UInt rhs)
        {
            const __m128i lhsSigned = Internal::RemapUnsigned(lhs.m_simdVector);
            const __m128i rhsSigned = Internal::RemapUnsigned(rhs.m_simdVector);
            const __m128i gtMask = _mm_cmpgt_epi32(lhsSigned, rhsSigned);
            return Internal::MoveMask(Internal::FlipMask(gtMask)) & 0b111;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL CmpGreaterEqualMask(const Vector3UInt lhs,
                                                                                         const Vector3UInt rhs)
        {
            const __m128i lhsSigned = Internal::RemapUnsigned(lhs.m_simdVector);
            const __m128i rhsSigned = Internal::RemapUnsigned(rhs.m_simdVector);
            const __m128i ltMask = _mm_cmplt_epi32(lhsSigned, rhsSigned);
            return Internal::MoveMask(Internal::FlipMask(ltMask)) & 0b111;
        }
    } // namespace Math


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator==(const Vector3UInt lhs, const Vector3UInt rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) == 0;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator!=(const Vector3UInt lhs, const Vector3UInt rhs)
    {
        return Math::CmpNotEqualMask(lhs, rhs) != 0;
    }
} // namespace FE
