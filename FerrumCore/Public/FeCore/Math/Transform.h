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
            FE_Assert(Math::CmpEqual(scale.x, scale.y) && Math::CmpEqual(scale.y, scale.z));
            FE_Assert(Math::CmpEqual(shear, Vector3::kZero));

            Transform transform;
            transform.m_translationScale = Vector4{ translation, scale.x };
            transform.m_rotation = rotation;
            return transform;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL ToMatrix(const Transform& transform)
        {
            const Vector4 scale = Vector4::BroadcastW(transform.m_translationScale);

            Matrix4x4 matrix = Matrix4x4::Rotation(transform.m_rotation);
            matrix.m_rows[0] *= scale;
            matrix.m_rows[1] *= scale;
            matrix.m_rows[2] *= scale;
            matrix.m_rows[3].m_simdVector =
                _mm_blend_ps(transform.m_translationScale.m_simdVector, Simd::SSE::Constants::kAllOnes, 0x7);
            return matrix;
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


    namespace Math
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Transform FE_VECTORCALL Invert(const Transform& source)
        {
            Transform transform;
            transform.m_rotation = Invert(source.m_rotation);

            const Vector4 scaleInverse = Vector4::BroadcastW(Reciprocal(source.m_translationScale));

            const Vector3 translationInverse =
                -Rotate(Vector4::GetXYZ(source.m_translationScale * scaleInverse), transform.m_rotation);

            transform.m_translationScale.m_simdVector =
                _mm_blend_ps(translationInverse.m_simdVector, scaleInverse.m_simdVector, 0b1110);
            return transform;
        }
    } // namespace Math
} // namespace FE

FE_RTTI_Reflect(FE::Transform, "27C520A6-9608-453D-BE4E-DCA551251325");
