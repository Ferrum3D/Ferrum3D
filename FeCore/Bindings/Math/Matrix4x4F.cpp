#include <FeCore/Console/FeLog.h>
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
            *result = l * r;
        }

        FE_DLL_EXPORT void Matrix4x4F_VectorMultiply(const Matrix4x4F* matrix, const Vector4F* vector, Vector4F* result)
        {
            Matrix4x4F m = *matrix;
            Vector4F v = *vector;
            *result = m * v;
        }

        FE_DLL_EXPORT float Matrix4x4F_Determinant(const Matrix4x4F* self)
        {
            return self->Determinant();
        }

        FE_DLL_EXPORT void Matrix4x4F_InverseTransform(const Matrix4x4F* self, Matrix4x4F* result)
        {
            Matrix4x4F m = *self;
            *result = m.InverseTransform();
        }
    }
} // namespace FE
