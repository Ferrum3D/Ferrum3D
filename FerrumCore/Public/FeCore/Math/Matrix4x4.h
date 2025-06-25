#pragma once
#include <FeCore/Math/Quaternion.h>
#include <FeCore/Math/Vector4.h>

namespace FE
{
    namespace Internal
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void Transpose4Impl(const __m128 row0, const __m128 row1, const __m128 row2,
                                                                  const __m128 row3, __m128* out)
        {
            const __m128 t0 = _mm_shuffle_ps(row0, row1, _MM_SHUFFLE(1, 0, 1, 0));
            const __m128 t1 = _mm_shuffle_ps(row0, row1, _MM_SHUFFLE(3, 2, 3, 2));
            const __m128 t2 = _mm_shuffle_ps(row2, row3, _MM_SHUFFLE(1, 0, 1, 0));
            const __m128 t3 = _mm_shuffle_ps(row2, row3, _MM_SHUFFLE(3, 2, 3, 2));
            out[0] = _mm_shuffle_ps(t0, t2, _MM_SHUFFLE(2, 0, 2, 0));
            out[1] = _mm_shuffle_ps(t0, t2, _MM_SHUFFLE(3, 1, 3, 1));
            out[2] = _mm_shuffle_ps(t1, t3, _MM_SHUFFLE(2, 0, 2, 0));
            out[3] = _mm_shuffle_ps(t1, t3, _MM_SHUFFLE(3, 1, 3, 1));
        }

        extern alignas(Memory::kCacheLineSize) const float kIdentity4Values[16];
    } // namespace Internal

    struct Matrix4x4 final
    {
        union
        {
            Vector4 m_rows[4];
            __m128 m_simdVectors[4];
            float m_values[16];
            struct
            {
                float m_00, m_01, m_02, m_03;
                float m_10, m_11, m_12, m_13;
                float m_20, m_21, m_22, m_23;
                float m_30, m_31, m_32, m_33;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4(ForceInitType)
            : m_simdVectors{ _mm_setzero_ps(), _mm_setzero_ps(), _mm_setzero_ps(), _mm_setzero_ps() }
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float* FE_VECTORCALL RowMajorData()
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE const float* FE_VECTORCALL RowMajorData() const
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL operator[](const uint32_t index) const
        {
            return m_rows[index];
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL FromRows(const Vector4 row0, const Vector4 row1,
                                                                                      const Vector4 row2, const Vector4 row3)
        {
            Matrix4x4 result;
            result.m_rows[0] = row0;
            result.m_rows[1] = row1;
            result.m_rows[2] = row2;
            result.m_rows[3] = row3;
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL FromColumns(const Vector4 column0,
                                                                                         const Vector4 column1,
                                                                                         const Vector4 column2,
                                                                                         const Vector4 column3)
        {
            Matrix4x4 result;
            Internal::Transpose4Impl(
                column0.m_simdVector, column1.m_simdVector, column2.m_simdVector, column3.m_simdVector, result.m_simdVectors);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL Zero()
        {
            return Matrix4x4(kForceInit);
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL LoadUnaligned(const float* values)
        {
            Matrix4x4 result;
            result.m_simdVectors[0] = _mm_loadu_ps(&values[0]);
            result.m_simdVectors[1] = _mm_loadu_ps(&values[4]);
            result.m_simdVectors[2] = _mm_loadu_ps(&values[8]);
            result.m_simdVectors[3] = _mm_loadu_ps(&values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL LoadAligned(const float* values)
        {
            FE_AssertDebug((reinterpret_cast<uintptr_t>(values) & 15) == 0);

            Matrix4x4 result;
            result.m_simdVectors[0] = _mm_load_ps(&values[0]);
            result.m_simdVectors[1] = _mm_load_ps(&values[4]);
            result.m_simdVectors[2] = _mm_load_ps(&values[8]);
            result.m_simdVectors[3] = _mm_load_ps(&values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL Identity()
        {
            Matrix4x4 result;
            result.m_simdVectors[0] = _mm_load_ps(&Internal::kIdentity4Values[0]);
            result.m_simdVectors[1] = _mm_load_ps(&Internal::kIdentity4Values[4]);
            result.m_simdVectors[2] = _mm_load_ps(&Internal::kIdentity4Values[8]);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL RotationX(const float angle)
        {
            const float sin = Math::Sin(angle);
            const float cos = Math::Cos(angle);

            Matrix4x4 result;
            result.m_simdVectors[0] = _mm_load_ps(&Internal::kIdentity4Values[0]);
            result.m_simdVectors[1] = _mm_setr_ps(0.0f, cos, sin, 0.0f);
            result.m_simdVectors[2] = _mm_setr_ps(0.0f, -sin, cos, 0.0f);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL RotationY(const float angle)
        {
            const float sin = Math::Sin(angle);
            const float cos = Math::Cos(angle);

            Matrix4x4 result;
            result.m_simdVectors[0] = _mm_setr_ps(cos, 0.0f, -sin, 0.0f);
            result.m_simdVectors[1] = _mm_load_ps(&Internal::kIdentity4Values[4]);
            result.m_simdVectors[2] = _mm_setr_ps(sin, 0.0f, cos, 0.0f);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL RotationZ(const float angle)
        {
            const float sin = Math::Sin(angle);
            const float cos = Math::Cos(angle);

            Matrix4x4 result;
            result.m_simdVectors[0] = _mm_setr_ps(cos, sin, 0.0f, 0.0f);
            result.m_simdVectors[1] = _mm_setr_ps(-sin, cos, 0.0f, 0.0f);
            result.m_simdVectors[2] = _mm_load_ps(&Internal::kIdentity4Values[8]);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL Rotation(const Quaternion quat)
        {
            //
            // From DirectXMath.
            //

            __m128 q0 = _mm_add_ps(quat.m_simdVector, quat.m_simdVector);
            __m128 q1 = _mm_mul_ps(quat.m_simdVector, q0);

            __m128 v0 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 0, 0, 1));
            v0 = _mm_and_ps(v0, SIMD::SSE::Masks::kFloatXYZ);
            __m128 v1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 1, 2, 2));
            v1 = _mm_and_ps(v1, SIMD::SSE::Masks::kFloatXYZ);
            __m128 r0 = _mm_sub_ps(SIMD::SSE::Constants::kFloat1110, v0);
            r0 = _mm_sub_ps(r0, v1);

            v0 = _mm_shuffle_ps(quat.m_simdVector, quat.m_simdVector, _MM_SHUFFLE(3, 1, 0, 0));
            v1 = _mm_shuffle_ps(q0, q0, _MM_SHUFFLE(3, 2, 1, 2));
            v0 = _mm_mul_ps(v0, v1);

            v1 = _mm_shuffle_ps(quat.m_simdVector, quat.m_simdVector, _MM_SHUFFLE(3, 3, 3, 3));
            __m128 v2 = _mm_shuffle_ps(q0, q0, _MM_SHUFFLE(3, 0, 2, 1));
            v1 = _mm_mul_ps(v1, v2);

            __m128 r1 = _mm_add_ps(v0, v1);
            __m128 r2 = _mm_sub_ps(v0, v1);

            v0 = _mm_shuffle_ps(r1, r2, _MM_SHUFFLE(1, 0, 2, 1));
            v0 = _mm_shuffle_ps(v0, v0, _MM_SHUFFLE(1, 3, 2, 0));
            v1 = _mm_shuffle_ps(r1, r2, _MM_SHUFFLE(2, 2, 0, 0));
            v1 = _mm_shuffle_ps(v1, v1, _MM_SHUFFLE(2, 0, 2, 0));

            q1 = _mm_shuffle_ps(r0, v0, _MM_SHUFFLE(1, 0, 3, 0));
            q1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 3, 2, 0));

            Matrix4x4 result;
            result.m_simdVectors[0] = q1;

            q1 = _mm_shuffle_ps(r0, v0, _MM_SHUFFLE(3, 2, 3, 1));
            q1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 3, 0, 2));
            result.m_simdVectors[1] = q1;

            q1 = _mm_shuffle_ps(v1, r0, _MM_SHUFFLE(3, 2, 1, 0));
            result.m_simdVectors[2] = q1;
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL Scale(const Vector3 scale)
        {
            const __m128 zero = _mm_setzero_ps();

            Matrix4x4 result;
            result.m_simdVectors[0] = _mm_blend_ps(zero, scale.m_simdVector, 0x1);
            result.m_simdVectors[1] = _mm_blend_ps(zero, scale.m_simdVector, 0x2);
            result.m_simdVectors[2] = _mm_blend_ps(zero, scale.m_simdVector, 0x4);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL Translation(const Vector3 translation)
        {
            Matrix4x4 result;
            result.m_simdVectors[0] = _mm_load_ps(&Internal::kIdentity4Values[0]);
            result.m_simdVectors[1] = _mm_load_ps(&Internal::kIdentity4Values[4]);
            result.m_simdVectors[2] = _mm_load_ps(&Internal::kIdentity4Values[8]);
            result.m_simdVectors[3] = _mm_blend_ps(_mm_load_ps(&Internal::kIdentity4Values[12]), translation.m_simdVector, 0x7);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL LookAt(const Vector3 position, const Vector3 target,
                                                                                    const Vector3 up)
        {
            const Vector3 z = Math::Normalize(target - position);
            const Vector3 x = Math::Normalize(Math::Cross(up, z));
            const Vector3 y = Math::Cross(z, x);

            const Vector3 negativePosition = -position;

            const __m128 d0 = _mm_set1_ps(Math::Dot(x, negativePosition));
            const __m128 d1 = _mm_set1_ps(Math::Dot(y, negativePosition));
            const __m128 d2 = _mm_set1_ps(Math::Dot(z, negativePosition));

            Matrix4x4 result;
            result.m_simdVectors[0] = _mm_blend_ps(d0, x.m_simdVector, 0x7);
            result.m_simdVectors[1] = _mm_blend_ps(d1, y.m_simdVector, 0x7);
            result.m_simdVectors[2] = _mm_blend_ps(d2, z.m_simdVector, 0x7);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);

            Internal::Transpose4Impl(result.m_simdVectors[0],
                                     result.m_simdVectors[1],
                                     result.m_simdVectors[2],
                                     result.m_simdVectors[3],
                                     result.m_simdVectors);

            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL Projection(const float fovY, const float aspectRatio,
                                                                                        const float near, const float far)
        {
            const float cotY = Math::Cos(0.5f * fovY) / Math::Sin(0.5f * fovY);
            const float cotX = cotY / aspectRatio;
            const float fRange = near / (far - near);

            Matrix4x4 result;
            result.m_simdVectors[0] = _mm_setr_ps(cotX, 0.0f, 0.0f, 0.0f);
            result.m_simdVectors[1] = _mm_setr_ps(0.0f, cotY, 0.0f, 0.0f);
            result.m_simdVectors[2] = _mm_setr_ps(0.0f, 0.0f, -fRange, 1.0f);
            result.m_simdVectors[3] = _mm_setr_ps(0.0f, 0.0f, far * fRange, 0.0f);
            return result;
        }
    };


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4 FE_VECTORCALL operator+(const Matrix4x4& lhs, const Matrix4x4& rhs)
    {
        Matrix4x4 result;
        result.m_simdVectors[0] = _mm_add_ps(lhs.m_simdVectors[0], rhs.m_simdVectors[0]);
        result.m_simdVectors[1] = _mm_add_ps(lhs.m_simdVectors[1], rhs.m_simdVectors[1]);
        result.m_simdVectors[2] = _mm_add_ps(lhs.m_simdVectors[2], rhs.m_simdVectors[2]);
        result.m_simdVectors[3] = _mm_add_ps(lhs.m_simdVectors[3], rhs.m_simdVectors[3]);
        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4& FE_VECTORCALL operator+=(Matrix4x4& lhs, const Matrix4x4& rhs)
    {
        return lhs = lhs + rhs;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4 FE_VECTORCALL operator-(const Matrix4x4& lhs, const Matrix4x4& rhs)
    {
        Matrix4x4 result;
        result.m_simdVectors[0] = _mm_sub_ps(lhs.m_simdVectors[0], rhs.m_simdVectors[0]);
        result.m_simdVectors[1] = _mm_sub_ps(lhs.m_simdVectors[1], rhs.m_simdVectors[1]);
        result.m_simdVectors[2] = _mm_sub_ps(lhs.m_simdVectors[2], rhs.m_simdVectors[2]);
        result.m_simdVectors[3] = _mm_sub_ps(lhs.m_simdVectors[3], rhs.m_simdVectors[3]);
        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4& FE_VECTORCALL operator-=(Matrix4x4& lhs, const Matrix4x4& rhs)
    {
        return lhs = lhs - rhs;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4 FE_VECTORCALL operator-(const Matrix4x4& mat)
    {
        Matrix4x4 result;
        result.m_rows[0] = -mat.m_rows[0];
        result.m_rows[1] = -mat.m_rows[1];
        result.m_rows[2] = -mat.m_rows[2];
        result.m_rows[3] = -mat.m_rows[3];
        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4 FE_VECTORCALL operator*(const Matrix4x4& lhs, const Matrix4x4& rhs)
    {
        Matrix4x4 result;

        // Splat the component X,Y,Z then W
        __m128 vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[0]) + 0);
        __m128 vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[0]) + 1);
        __m128 vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[0]) + 2);
        __m128 vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[0]) + 3);

        vX = _mm_mul_ps(vX, rhs.m_simdVectors[0]);
        vY = _mm_mul_ps(vY, rhs.m_simdVectors[1]);
        vZ = _mm_mul_ps(vZ, rhs.m_simdVectors[2]);
        vW = _mm_mul_ps(vW, rhs.m_simdVectors[3]);
        vX = _mm_add_ps(vX, vZ);
        vY = _mm_add_ps(vY, vW);
        vX = _mm_add_ps(vX, vY);
        result.m_simdVectors[0] = vX;

        vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[1]) + 0);
        vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[1]) + 1);
        vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[1]) + 2);
        vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[1]) + 3);

        vX = _mm_mul_ps(vX, rhs.m_simdVectors[0]);
        vY = _mm_mul_ps(vY, rhs.m_simdVectors[1]);
        vZ = _mm_mul_ps(vZ, rhs.m_simdVectors[2]);
        vW = _mm_mul_ps(vW, rhs.m_simdVectors[3]);
        vX = _mm_add_ps(vX, vZ);
        vY = _mm_add_ps(vY, vW);
        vX = _mm_add_ps(vX, vY);
        result.m_simdVectors[1] = vX;

        vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[2]) + 0);
        vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[2]) + 1);
        vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[2]) + 2);
        vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[2]) + 3);

        vX = _mm_mul_ps(vX, rhs.m_simdVectors[0]);
        vY = _mm_mul_ps(vY, rhs.m_simdVectors[1]);
        vZ = _mm_mul_ps(vZ, rhs.m_simdVectors[2]);
        vW = _mm_mul_ps(vW, rhs.m_simdVectors[3]);
        vX = _mm_add_ps(vX, vZ);
        vY = _mm_add_ps(vY, vW);
        vX = _mm_add_ps(vX, vY);
        result.m_simdVectors[2] = vX;

        vX = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[3]) + 0);
        vY = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[3]) + 1);
        vZ = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[3]) + 2);
        vW = _mm_broadcast_ss(reinterpret_cast<const float*>(&lhs.m_simdVectors[3]) + 3);

        vX = _mm_mul_ps(vX, rhs.m_simdVectors[0]);
        vY = _mm_mul_ps(vY, rhs.m_simdVectors[1]);
        vZ = _mm_mul_ps(vZ, rhs.m_simdVectors[2]);
        vW = _mm_mul_ps(vW, rhs.m_simdVectors[3]);
        vX = _mm_add_ps(vX, vZ);
        vY = _mm_add_ps(vY, vW);
        vX = _mm_add_ps(vX, vY);
        result.m_simdVectors[3] = vX;

        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4& FE_VECTORCALL operator*=(Matrix4x4& lhs, const Matrix4x4& rhs)
    {
        return lhs = lhs * rhs;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL operator*(const Vector4 lhs, const Matrix4x4& rhs)
    {
        return Vector4::BroadcastW(lhs) * rhs.m_rows[3] + Vector4::BroadcastZ(lhs) * rhs.m_rows[2]
            + Vector4::BroadcastY(lhs) * rhs.m_rows[1] + Vector4::BroadcastX(lhs) * rhs.m_rows[0];
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4 FE_VECTORCALL operator*(const Matrix4x4& lhs, const float rhs)
    {
        const __m128 broadcast = _mm_set1_ps(rhs);

        Matrix4x4 result;
        result.m_simdVectors[0] = _mm_mul_ps(lhs.m_simdVectors[0], broadcast);
        result.m_simdVectors[1] = _mm_mul_ps(lhs.m_simdVectors[1], broadcast);
        result.m_simdVectors[2] = _mm_mul_ps(lhs.m_simdVectors[2], broadcast);
        result.m_simdVectors[3] = _mm_mul_ps(lhs.m_simdVectors[3], broadcast);
        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4& FE_VECTORCALL operator*=(Matrix4x4& lhs, const float rhs)
    {
        return lhs = lhs * rhs;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4 FE_VECTORCALL operator/(const Matrix4x4& lhs, const float rhs)
    {
        const __m128 broadcast = _mm_set1_ps(rhs);

        Matrix4x4 result;
        result.m_simdVectors[0] = _mm_div_ps(lhs.m_simdVectors[0], broadcast);
        result.m_simdVectors[1] = _mm_div_ps(lhs.m_simdVectors[1], broadcast);
        result.m_simdVectors[2] = _mm_div_ps(lhs.m_simdVectors[2], broadcast);
        result.m_simdVectors[3] = _mm_div_ps(lhs.m_simdVectors[3], broadcast);
        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4& FE_VECTORCALL operator/=(Matrix4x4& lhs, const float rhs)
    {
        return lhs = lhs / rhs;
    }


    namespace Math
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4 FE_VECTORCALL Transpose(const Matrix4x4& matrix)
        {
            Matrix4x4 result;
            FE::Internal::Transpose4Impl(matrix.m_simdVectors[0],
                                         matrix.m_simdVectors[1],
                                         matrix.m_simdVectors[2],
                                         matrix.m_simdVectors[3],
                                         result.m_simdVectors);
            return result;
        }


        FE_NO_SECURITY_COOKIE Matrix4x4 FE_VECTORCALL InverseTransform(const Matrix4x4& matrix);


        FE_NO_SECURITY_COOKIE bool FE_VECTORCALL DecomposeTransform(const Matrix4x4& matrix, Vector3& translation,
                                                                    Quaternion& rotation, Vector3& scale, Vector3& shear);


        FE_NO_SECURITY_COOKIE Quaternion FE_VECTORCALL ExtractRotation(const Matrix4x4& matrix);


        template<uint32_t TColumnIndex>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4 FE_VECTORCALL ExtractColumn(const Matrix4x4& matrix)
        {
            constexpr int32_t shuffleMask = _MM_SHUFFLE(TColumnIndex, TColumnIndex, TColumnIndex, TColumnIndex);
            const __m128 x = _mm_shuffle_ps(matrix.m_simdVectors[0], matrix.m_simdVectors[0], shuffleMask);
            const __m128 y = _mm_shuffle_ps(matrix.m_simdVectors[1], matrix.m_simdVectors[1], shuffleMask);
            const __m128 z = _mm_shuffle_ps(matrix.m_simdVectors[2], matrix.m_simdVectors[2], shuffleMask);
            const __m128 w = _mm_shuffle_ps(matrix.m_simdVectors[3], matrix.m_simdVectors[3], shuffleMask);
            return Vector4{ _mm_blend_ps(_mm_blend_ps(_mm_blend_ps(x, y, 0x2), z, 0xc), w, 0x8) };
        }


        template<uint32_t TColumnIndex>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void FE_VECTORCALL ReplaceColumn(Matrix4x4& matrix, const Vector4 column)
        {
            constexpr uint32_t sourceMask = TColumnIndex << 4u;
            matrix.m_simdVectors[0] = _mm_insert_ps(matrix.m_simdVectors[0], column.m_simdVector, sourceMask | (0 << 6));
            matrix.m_simdVectors[1] = _mm_insert_ps(matrix.m_simdVectors[1], column.m_simdVector, sourceMask | (1 << 6));
            matrix.m_simdVectors[2] = _mm_insert_ps(matrix.m_simdVectors[2], column.m_simdVector, sourceMask | (2 << 6));
            matrix.m_simdVectors[3] = _mm_insert_ps(matrix.m_simdVectors[3], column.m_simdVector, sourceMask | (3 << 6));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EqualEstimate(Matrix4x4 lhs, Matrix4x4 rhs,
                                                                               float epsilon = Constants::kEpsilon)
        {
            const __m128 kSignMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
            const __m128 epsilonBroadcast = _mm_set1_ps(epsilon);

            __m128 distance = _mm_and_ps(_mm_sub_ps(lhs.m_simdVectors[0], rhs.m_simdVectors[0]), kSignMask);
            __m128 mask = _mm_cmpgt_ps(distance, epsilonBroadcast);

            distance = _mm_and_ps(_mm_sub_ps(lhs.m_simdVectors[1], rhs.m_simdVectors[1]), kSignMask);
            mask = _mm_or_ps(mask, _mm_cmpgt_ps(distance, epsilonBroadcast));

            distance = _mm_and_ps(_mm_sub_ps(lhs.m_simdVectors[2], rhs.m_simdVectors[2]), kSignMask);
            mask = _mm_or_ps(mask, _mm_cmpgt_ps(distance, epsilonBroadcast));

            distance = _mm_and_ps(_mm_sub_ps(lhs.m_simdVectors[3], rhs.m_simdVectors[3]), kSignMask);
            mask = _mm_or_ps(mask, _mm_cmpgt_ps(distance, epsilonBroadcast));

            return _mm_movemask_ps(mask) == 0;
        }
    } // namespace Math
} // namespace FE
