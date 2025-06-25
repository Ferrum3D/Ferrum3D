#pragma once
#include <FeCore/Math/Matrix4x4.h>
#include <FeCore/Math/Quaternion.h>
#include <FeCore/Math/Vector4.h>

namespace FE
{
    struct Transform
    {
        Vector4 m_translationScale;
        Quaternion m_rotation;

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Transform() = default;

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE explicit Transform(ForceInitType)
            : m_translationScale(Vector4::AxisW())
            , m_rotation(Quaternion::Identity())
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3 FE_VECTORCALL Translation() const
        {
            return Vector3{ m_translationScale.m_simdVector };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Scale() const
        {
            return m_translationScale.w;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Quaternion FE_VECTORCALL Rotation() const
        {
            return m_rotation;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Transform FE_VECTORCALL Identity()
        {
            Transform transform;
            transform.m_translationScale = Vector4::AxisW();
            transform.m_rotation = Quaternion::Identity();
            return transform;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Transform FE_VECTORCALL Create(const Vector3 translation,
                                                                                    const Quaternion rotation, const float scale)
        {
            Transform transform;
            transform.m_translationScale = Vector4{ translation, scale };
            transform.m_rotation = rotation;
            return transform;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Transform FE_VECTORCALL FromMatrix(const Matrix4x4& matrix)
        {
            Vector3 translation;
            Quaternion rotation;
            Vector3 scale;
            Vector3 shear;

            FE_Verify(Math::DecomposeTransform(matrix, translation, rotation, scale, shear));
            FE_Assert(Math::EqualEstimate(scale.x, scale.y) && Math::EqualEstimate(scale.y, scale.z));
            FE_Assert(Math::EqualEstimate(shear, Vector3::Zero()));

            Transform transform;
            transform.m_translationScale = Vector4{ translation, scale.x };
            transform.m_rotation = rotation;
            return transform;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Transform FE_VECTORCALL Translation(const Vector3 translation)
        {
            Transform transform;
            transform.m_translationScale = Vector4{ translation, 1.0f };
            transform.m_rotation = Quaternion::Identity();
            return transform;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Transform FE_VECTORCALL Rotation(const Quaternion rotation)
        {
            Transform transform;
            transform.m_translationScale = Vector4{ 0.0f, 0.0f, 0.0f, 1.0f };
            transform.m_rotation = rotation;
            return transform;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Transform FE_VECTORCALL Scale(const float scale)
        {
            Transform transform;
            transform.m_translationScale = Vector4{ 0.0f, 0.0f, 0.0f, scale };
            transform.m_rotation = Quaternion::Identity();
            return transform;
        }
    };
} // namespace FE
