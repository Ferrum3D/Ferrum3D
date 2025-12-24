#pragma once
#include <FeCore/Math/Aabb.h>

namespace FE::Simd::Soa
{
    struct MaskX8 final
    {
        __m256 m_value;
    };


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE uint32_t FE_VECTORCALL MoveMask(const MaskX8 mask)
    {
        return _mm256_movemask_ps(mask.m_value);
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL Any(const MaskX8 mask)
    {
        return _mm256_movemask_ps(mask.m_value) != 0;
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL All(const MaskX8 mask)
    {
        return _mm256_movemask_ps(mask.m_value) == 0xff;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE MaskX8 FE_VECTORCALL And(const MaskX8 lhs, const MaskX8 rhs)
    {
        return { _mm256_and_ps(lhs.m_value, rhs.m_value) };
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE MaskX8 FE_VECTORCALL Or(const MaskX8 lhs, const MaskX8 rhs)
    {
        return { _mm256_or_ps(lhs.m_value, rhs.m_value) };
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE MaskX8 FE_VECTORCALL Xor(const MaskX8 lhs, const MaskX8 rhs)
    {
        return { _mm256_xor_ps(lhs.m_value, rhs.m_value) };
    }


    struct Vector3X8 final
    {
        __m256 x;
        __m256 y;
        __m256 z;
    };


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void FE_VECTORCALL SetZero(Vector3X8& vector)
    {
        vector.x = _mm256_setzero_ps();
        vector.y = _mm256_setzero_ps();
        vector.z = _mm256_setzero_ps();
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3X8 FE_VECTORCALL Broadcast(const Vector3 value)
    {
        return { _mm256_set1_ps(value.x), _mm256_set1_ps(value.y), _mm256_set1_ps(value.z) };
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void FE_VECTORCALL Insert(Vector3X8& vector, const int32_t index, const Vector3 value)
    {
        FE_CoreAssert(static_cast<uint32_t>(index) < AVX::kLaneCount);

        vector.x = AVX::InsertLane(vector.x, value.x, index);
        vector.y = AVX::InsertLane(vector.y, value.y, index);
        vector.z = AVX::InsertLane(vector.z, value.z, index);
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3 FE_VECTORCALL Extract(const Vector3X8& vector, const int32_t index)
    {
        FE_CoreAssert(static_cast<uint32_t>(index) < AVX::kLaneCount);
        return { AVX::ExtractLane(vector.x, index), AVX::ExtractLane(vector.y, index), AVX::ExtractLane(vector.z, index) };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE MaskX8 FE_VECTORCALL AllLessThan(const Vector3X8& lhs, const Vector3X8& rhs)
    {
        const __m256 x = _mm256_cmp_ps(lhs.x, rhs.x, _CMP_LT_OQ);
        const __m256 y = _mm256_cmp_ps(lhs.y, rhs.y, _CMP_LT_OQ);
        const __m256 z = _mm256_cmp_ps(lhs.z, rhs.z, _CMP_LT_OQ);
        return { _mm256_and_ps(x, _mm256_and_ps(y, z)) };
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE MaskX8 FE_VECTORCALL AllGreaterThan(const Vector3X8& lhs, const Vector3X8& rhs)
    {
        const __m256 x = _mm256_cmp_ps(lhs.x, rhs.x, _CMP_GT_OQ);
        const __m256 y = _mm256_cmp_ps(lhs.y, rhs.y, _CMP_GT_OQ);
        const __m256 z = _mm256_cmp_ps(lhs.z, rhs.z, _CMP_GT_OQ);
        return { _mm256_and_ps(x, _mm256_and_ps(y, z)) };
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE MaskX8 FE_VECTORCALL AllLessEqual(const Vector3X8& lhs, const Vector3X8& rhs)
    {
        const __m256 x = _mm256_cmp_ps(lhs.x, rhs.x, _CMP_LE_OQ);
        const __m256 y = _mm256_cmp_ps(lhs.y, rhs.y, _CMP_LE_OQ);
        const __m256 z = _mm256_cmp_ps(lhs.z, rhs.z, _CMP_LE_OQ);
        return { _mm256_and_ps(x, _mm256_and_ps(y, z)) };
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE MaskX8 FE_VECTORCALL AllGreaterEqual(const Vector3X8& lhs, const Vector3X8& rhs)
    {
        const __m256 x = _mm256_cmp_ps(lhs.x, rhs.x, _CMP_GE_OQ);
        const __m256 y = _mm256_cmp_ps(lhs.y, rhs.y, _CMP_GE_OQ);
        const __m256 z = _mm256_cmp_ps(lhs.z, rhs.z, _CMP_GE_OQ);
        return { _mm256_and_ps(x, _mm256_and_ps(y, z)) };
    }


    struct AabbX8 final
    {
        Vector3X8 min;
        Vector3X8 max;
    };


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void FE_VECTORCALL SetZero(AabbX8& aabb)
    {
        SetZero(aabb.min);
        SetZero(aabb.max);
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE AabbX8 FE_VECTORCALL Broadcast(const Aabb& value)
    {
        return { Broadcast(value.min), Broadcast(value.max) };
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void FE_VECTORCALL Insert(AabbX8& aabb, const int32_t index, const Aabb& source)
    {
        Insert(aabb.min, index, source.min);
        Insert(aabb.max, index, source.max);
    }

    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Aabb FE_VECTORCALL Extract(const AabbX8& aabb, const int32_t index)
    {
        return { Extract(aabb.min, index), Extract(aabb.max, index) };
    }
} // namespace FE::Simd::Soa
