#pragma once
#include <FeCore/Base/BaseTypes.h>
#include <FeCore/Math/Vector4.h>

namespace FE
{
    struct Sphere final
    {
        union
        {
            __m128 m_simdVector;
            Vector4 m_centerRadius;
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Sphere() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Sphere(ForceInitType)
            : m_simdVector(_mm_setzero_ps())
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Sphere(const Vector3 center, const float radius)
            : m_centerRadius(Vector4(center, radius))
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3 Center() const
        {
            return Vector3{ m_centerRadius.m_simdVector };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Radius() const
        {
            return m_centerRadius.w;
        }

        static const Sphere kZero;
    };

    inline const Sphere Sphere::kZero{ kForceInit };


    namespace Math
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL Overlaps(const Sphere& lhs, const Sphere& rhs)
        {
            const Vector3 diff = lhs.Center() - rhs.Center();
            const float radiusSum = lhs.Radius() + rhs.Radius();
            return LengthSquared(diff) <= radiusSum * radiusSum;
        }
    } // namespace Math
} // namespace FE

FE_RTTI_Reflect(FE::Sphere, "610C03BA-7484-4A59-B85E-67595ACC6BA4");
