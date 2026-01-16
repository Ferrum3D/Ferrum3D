#pragma once
#include <FeCore/Base/BaseTypes.h>
#include <FeCore/Math/Aabb.h>
#include <FeCore/Math/Quaternion.h>

namespace FE
{
    struct Obb final
    {
        Vector3 center;
        Vector3 extents;
        Quaternion rotation;

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Obb() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Obb(ForceInitType)
            : center(kForceInit)
            , extents(kForceInit)
            , rotation(kForceInit)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Obb(const Vector3 center, const Vector3 extents,
                                                  const Quaternion rotation = Quaternion::Identity())
            : center(center)
            , extents(extents)
            , rotation(rotation)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Obb(const Aabb& aabb)
            : center((aabb.min + aabb.max) * 0.5f)
            , extents((aabb.max - aabb.min) * 0.5f)
            , rotation(Quaternion::Identity())
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Obb(const Aabb& aabb, const Quaternion rotation)
            : center((aabb.min + aabb.max) * 0.5f)
            , extents((aabb.max - aabb.min) * 0.5f)
            , rotation(rotation)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3 FE_VECTORCALL Size() const
        {
            return extents * 2.0f;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL IsValid() const
        {
            return Math::CmpLessMask(extents, Vector3::kZero) == 0;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3 FE_VECTORCALL AxisX() const
        {
            return Math::Rotate(Vector3::WorldRight(), rotation);
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3 FE_VECTORCALL AxisY() const
        {
            return Math::Rotate(Vector3::WorldUp(), rotation);
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3 FE_VECTORCALL AxisZ() const
        {
            return Math::Rotate(Vector3::WorldForward(), rotation);
        }

        static const Obb kZero;
    };

    inline const Obb Obb::kZero{ kForceInit };


    namespace Math
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Aabb FE_VECTORCALL ToAabb(const Obb& obb)
        {
            const Vector3 axisX = Rotate(Vector3::WorldRight(), obb.rotation);
            const Vector3 axisY = Rotate(Vector3::WorldUp(), obb.rotation);
            const Vector3 axisZ = Rotate(Vector3::WorldForward(), obb.rotation);

            const Vector3 absX = Abs(axisX) * obb.extents.x;
            const Vector3 absY = Abs(axisY) * obb.extents.y;
            const Vector3 absZ = Abs(axisZ) * obb.extents.z;

            const Vector3 aabbExtents = absX + absY + absZ;
            return Aabb{ obb.center - aabbExtents, obb.center + aabbExtents };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL CmpEqual(const Obb& lhs, const Obb& rhs,
                                                                          const float epsilon = Constants::kEpsilon)
        {
            return CmpEqual(lhs.center, rhs.center, epsilon) && CmpEqual(lhs.extents, rhs.extents, epsilon)
                && CmpEqual(lhs.rotation, rhs.rotation, epsilon);
        }
    } // namespace Math
} // namespace FE

FE_RTTI_Reflect(FE::Obb, "71B0B56A-6A5E-4F22-B062-64E4BA2D9EA0");
