#pragma once
#include <Core/Base/BaseMath.h>
#include <Core/Math/Frustum.h>
#include <Core/SIMD/Soa.h>

namespace FE::Simd::Intersection
{
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Soa::MaskX8 FE_VECTORCALL Overlaps(const Frustum& frustum, const Soa::AabbX8& bounds)
    {
        __m256 mask = _mm256_castsi256_ps(_mm256_set1_epi32(-1));
        for (const Vector4& plane : frustum.m_planes)
        {
            const __m256 x = plane.x >= 0.0f ? bounds.max.x : bounds.min.x;
            const __m256 y = plane.y >= 0.0f ? bounds.max.y : bounds.min.y;
            const __m256 z = plane.z >= 0.0f ? bounds.max.z : bounds.min.z;
            __m256 distance = _mm256_mul_ps(_mm256_set1_ps(plane.x), x);
            distance = _mm256_add_ps(distance, _mm256_mul_ps(_mm256_set1_ps(plane.y), y));
            distance = _mm256_add_ps(distance, _mm256_mul_ps(_mm256_set1_ps(plane.z), z));
            distance = _mm256_add_ps(distance, _mm256_set1_ps(plane.w));
            mask = _mm256_and_ps(mask, _mm256_cmp_ps(distance, _mm256_setzero_ps(), _CMP_GE_OQ));
        }

        return { mask };
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Soa::MaskX8 FE_VECTORCALL Overlaps(const Soa::AabbX8& lhs, const Soa::AabbX8& rhs)
    {
        const Soa::MaskX8 mask1 = Soa::AllLessEqual(lhs.min, rhs.max);
        const Soa::MaskX8 mask2 = Soa::AllGreaterEqual(lhs.max, rhs.min);
        return Soa::And(mask1, mask2);
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Soa::MaskX8 FE_VECTORCALL Contains(const Soa::AabbX8& lhs, const Soa::AabbX8& rhs)
    {
        const Soa::MaskX8 mask1 = Soa::AllGreaterEqual(rhs.min, lhs.min);
        const Soa::MaskX8 mask2 = Soa::AllLessEqual(rhs.max, lhs.max);
        return Soa::And(mask1, mask2);
    }
} // namespace FE::Simd::Intersection
