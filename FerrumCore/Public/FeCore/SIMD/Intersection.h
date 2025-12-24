#pragma once
#include <FeCore/Base/BaseMath.h>
#include <FeCore/SIMD/Soa.h>

namespace FE::Simd::Intersection
{
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
