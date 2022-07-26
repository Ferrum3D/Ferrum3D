#include <FeCore/Math/Matrix4x4F.h>

namespace FE
{
    extern "C"
    {
        FE_DLL_EXPORT void Matrix4x4F_Multiply(const Matrix4x4F* lhs, const Matrix4x4F* rhs, Matrix4x4F* result)
        {
            // Need to copy, because SIMD requires 16-byte alignment, which is not guaranteed by .Net runtime.
            Matrix4x4F l = *lhs;
            Matrix4x4F r = *rhs;
            *result      = l * r;
        }

        FE_DLL_EXPORT void Matrix4x4F_CreateRotationX(Float32 angle, Matrix4x4F* result)
        {
            *result = Matrix4x4F::CreateRotationX(angle);
        }

        FE_DLL_EXPORT void Matrix4x4F_CreateRotationY(Float32 angle, Matrix4x4F* result)
        {
            *result = Matrix4x4F::CreateRotationY(angle);
        }

        FE_DLL_EXPORT void Matrix4x4F_CreateRotationZ(Float32 angle, Matrix4x4F* result)
        {
            *result = Matrix4x4F::CreateRotationZ(angle);
        }

        FE_DLL_EXPORT void Matrix4x4F_CreateRotation(const Quaternion* quaternion, Matrix4x4F* result)
        {
            auto q  = *quaternion;
            *result = Matrix4x4F::CreateRotation(q);
        }

        FE_DLL_EXPORT void Matrix4x4F_CreateTransform(const Quaternion* quaternion, const Vector4F* position, Matrix4x4F* result)
        {
            auto q  = *quaternion;
            auto p  = *position;
            *result = Matrix4x4F::CreateTransform(q, p.GetVector3F());
        }

        FE_DLL_EXPORT void Matrix4x4F_VectorMultiply(const Matrix4x4F* matrix, const Vector4F* vector, Vector4F* result)
        {
            Matrix4x4F m = *matrix;
            Vector4F v   = *vector;
            *result      = m * v;
        }

        FE_DLL_EXPORT float Matrix4x4F_Determinant(const Matrix4x4F* self)
        {
            return self->Determinant();
        }

        FE_DLL_EXPORT void Matrix4x4F_InverseTransform(const Matrix4x4F* self, Matrix4x4F* result)
        {
            Matrix4x4F m = *self;
            *result      = m.InverseTransform();
        }
    }
} // namespace FE
