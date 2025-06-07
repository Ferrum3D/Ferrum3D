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

    struct Matrix4x4F final
    {
        union
        {
            Vector4F m_rows[4];
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

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4F() = default;

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float* FE_VECTORCALL RowMajorData()
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE const float* FE_VECTORCALL RowMajorData() const
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL FromRows(const Vector4F row0, const Vector4F row1,
                                                                                       const Vector4F row2, const Vector4F row3)
        {
            Matrix4x4F result;
            result.m_rows[0] = row0;
            result.m_rows[1] = row1;
            result.m_rows[2] = row2;
            result.m_rows[3] = row3;
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL FromColumns(const Vector4F column0,
                                                                                          const Vector4F column1,
                                                                                          const Vector4F column2,
                                                                                          const Vector4F column3)
        {
            Matrix4x4F result;
            Internal::Transpose4Impl(
                column0.m_simdVector, column1.m_simdVector, column2.m_simdVector, column3.m_simdVector, result.m_simdVectors);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL Zero()
        {
            return FromRows(Vector4F::Zero(), Vector4F::Zero(), Vector4F::Zero(), Vector4F::Zero());
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL LoadUnaligned(const float* values)
        {
            Matrix4x4F result;
            result.m_simdVectors[0] = _mm_loadu_ps(&values[0]);
            result.m_simdVectors[1] = _mm_loadu_ps(&values[4]);
            result.m_simdVectors[2] = _mm_loadu_ps(&values[8]);
            result.m_simdVectors[3] = _mm_loadu_ps(&values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL LoadAligned(const float* values)
        {
            FE_AssertDebug((reinterpret_cast<uintptr_t>(values) & 15) == 0);

            Matrix4x4F result;
            result.m_simdVectors[0] = _mm_load_ps(&values[0]);
            result.m_simdVectors[1] = _mm_load_ps(&values[4]);
            result.m_simdVectors[2] = _mm_load_ps(&values[8]);
            result.m_simdVectors[3] = _mm_load_ps(&values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL Identity()
        {
            Matrix4x4F result;
            result.m_simdVectors[0] = _mm_load_ps(&Internal::kIdentity4Values[0]);
            result.m_simdVectors[1] = _mm_load_ps(&Internal::kIdentity4Values[4]);
            result.m_simdVectors[2] = _mm_load_ps(&Internal::kIdentity4Values[8]);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL RotationX(const float angle)
        {
            const float sin = Math::Sin(angle);
            const float cos = Math::Cos(angle);

            Matrix4x4F result;
            result.m_simdVectors[0] = _mm_load_ps(&Internal::kIdentity4Values[0]);
            result.m_simdVectors[1] = _mm_setr_ps(0.0f, cos, -sin, 0.0f);
            result.m_simdVectors[2] = _mm_setr_ps(0.0f, sin, cos, 0.0f);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL RotationY(const float angle)
        {
            const float sin = Math::Sin(angle);
            const float cos = Math::Cos(angle);

            Matrix4x4F result;
            result.m_simdVectors[0] = _mm_setr_ps(cos, 0.0f, sin, 0.0f);
            result.m_simdVectors[1] = _mm_load_ps(&Internal::kIdentity4Values[4]);
            result.m_simdVectors[2] = _mm_setr_ps(-sin, 0.0f, cos, 0.0f);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL RotationZ(const float angle)
        {
            const float sin = Math::Sin(angle);
            const float cos = Math::Cos(angle);

            Matrix4x4F result;
            result.m_simdVectors[0] = _mm_setr_ps(cos, -sin, 0.0f, 0.0f);
            result.m_simdVectors[1] = _mm_setr_ps(sin, cos, 0.0f, 0.0f);
            result.m_simdVectors[2] = _mm_load_ps(&Internal::kIdentity4Values[8]);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL Rotation(const Quaternion quat)
        {
            const float wx2 = 2.0f * quat.w * quat.x;
            const float wy2 = 2.0f * quat.w * quat.y;
            const float wz2 = 2.0f * quat.w * quat.z;
            const float xx2 = 2.0f * quat.x * quat.x;
            const float xy2 = 2.0f * quat.x * quat.y;
            const float xz2 = 2.0f * quat.x * quat.z;
            const float yy2 = 2.0f * quat.y * quat.y;
            const float yz2 = 2.0f * quat.y * quat.z;
            const float zz2 = 2.0f * quat.z * quat.z;

            Matrix4x4F result;
            result.m_simdVectors[0] = _mm_setr_ps(1.0f - yy2 - zz2, xy2 - wz2, xz2 + wy2, 0.0f);
            result.m_simdVectors[1] = _mm_setr_ps(xy2 + wz2, 1.0f - xx2 - zz2, yz2 - wx2, 0.0f);
            result.m_simdVectors[2] = _mm_setr_ps(xz2 - wy2, yz2 + wx2, 1.0f - xx2 - yy2, 0.0f);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL Scale(const Vector3F scale)
        {
            const __m128 zero = _mm_setzero_ps();

            Matrix4x4F result;
            result.m_simdVectors[0] = _mm_blend_ps(zero, scale.m_simdVector, 1);
            result.m_simdVectors[1] = _mm_blend_ps(zero, scale.m_simdVector, 2);
            result.m_simdVectors[2] = _mm_blend_ps(zero, scale.m_simdVector, 4);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL Translation(const Vector3F translation)
        {
            Matrix4x4F result;
            result.m_simdVectors[0] = _mm_insert_ps(_mm_load_ps(&Internal::kIdentity4Values[0]), translation.m_simdVector, 0x30);
            result.m_simdVectors[1] = _mm_insert_ps(_mm_load_ps(&Internal::kIdentity4Values[4]), translation.m_simdVector, 0x70);
            result.m_simdVectors[2] = _mm_insert_ps(_mm_load_ps(&Internal::kIdentity4Values[8]), translation.m_simdVector, 0xB0);
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[12]);
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4F FE_VECTORCALL Projection(const float fovY,
                                                                                         const float aspectRatio,
                                                                                         const float near, const float far)
        {
            const float cotY = Math::Cos(0.5f * fovY) / Math::Sin(0.5f * fovY);
            const float cotX = cotY / aspectRatio;
            const float invFl = 1.0f / (far - near);

            Matrix4x4F result;
            result.m_rows[0] = Vector4F::AxisX(-cotX);
            result.m_rows[1] = Vector4F::AxisY(cotY);
            result.m_rows[2] = Vector4F{ 0.0f, 0.0f, far * invFl, -far * near * invFl };
            result.m_simdVectors[3] = _mm_load_ps(&Internal::kIdentity4Values[8]);
            return result;
        }
    };


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4F FE_VECTORCALL operator+(const Matrix4x4F& lhs, const Matrix4x4F& rhs)
    {
        Matrix4x4F result;
        result.m_simdVectors[0] = _mm_add_ps(lhs.m_simdVectors[0], rhs.m_simdVectors[0]);
        result.m_simdVectors[1] = _mm_add_ps(lhs.m_simdVectors[1], rhs.m_simdVectors[1]);
        result.m_simdVectors[2] = _mm_add_ps(lhs.m_simdVectors[2], rhs.m_simdVectors[2]);
        result.m_simdVectors[3] = _mm_add_ps(lhs.m_simdVectors[3], rhs.m_simdVectors[3]);
        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4F FE_VECTORCALL operator-(const Matrix4x4F& lhs, const Matrix4x4F& rhs)
    {
        Matrix4x4F result;
        result.m_simdVectors[0] = _mm_sub_ps(lhs.m_simdVectors[0], rhs.m_simdVectors[0]);
        result.m_simdVectors[1] = _mm_sub_ps(lhs.m_simdVectors[1], rhs.m_simdVectors[1]);
        result.m_simdVectors[2] = _mm_sub_ps(lhs.m_simdVectors[2], rhs.m_simdVectors[2]);
        result.m_simdVectors[3] = _mm_sub_ps(lhs.m_simdVectors[3], rhs.m_simdVectors[3]);
        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4F FE_VECTORCALL operator-(const Matrix4x4F& mat)
    {
        Matrix4x4F result;
        result.m_rows[0] = -mat.m_rows[0];
        result.m_rows[1] = -mat.m_rows[1];
        result.m_rows[2] = -mat.m_rows[2];
        result.m_rows[3] = -mat.m_rows[3];
        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4F FE_VECTORCALL operator*(const Matrix4x4F& lhs, const Matrix4x4F& rhs)
    {
        Matrix4x4F result;

        result.m_rows[0] = Vector4F::Swizzle<Math::Swizzle::kWWWW>(lhs.m_rows[0]) * rhs.m_rows[3]
            + Vector4F::Swizzle<Math::Swizzle::kZZZZ>(lhs.m_rows[0]) * rhs.m_rows[2]
            + Vector4F::Swizzle<Math::Swizzle::kYYYY>(lhs.m_rows[0]) * rhs.m_rows[1]
            + Vector4F::Swizzle<Math::Swizzle::kXXXX>(lhs.m_rows[0]) * rhs.m_rows[0];

        result.m_rows[1] = Vector4F::Swizzle<Math::Swizzle::kWWWW>(lhs.m_rows[1]) * rhs.m_rows[3]
            + Vector4F::Swizzle<Math::Swizzle::kZZZZ>(lhs.m_rows[1]) * rhs.m_rows[2]
            + Vector4F::Swizzle<Math::Swizzle::kYYYY>(lhs.m_rows[1]) * rhs.m_rows[1]
            + Vector4F::Swizzle<Math::Swizzle::kXXXX>(lhs.m_rows[1]) * rhs.m_rows[0];

        result.m_rows[2] = Vector4F::Swizzle<Math::Swizzle::kWWWW>(lhs.m_rows[2]) * rhs.m_rows[3]
            + Vector4F::Swizzle<Math::Swizzle::kZZZZ>(lhs.m_rows[2]) * rhs.m_rows[2]
            + Vector4F::Swizzle<Math::Swizzle::kYYYY>(lhs.m_rows[2]) * rhs.m_rows[1]
            + Vector4F::Swizzle<Math::Swizzle::kXXXX>(lhs.m_rows[2]) * rhs.m_rows[0];

        result.m_rows[3] = Vector4F::Swizzle<Math::Swizzle::kWWWW>(lhs.m_rows[3]) * rhs.m_rows[3]
            + Vector4F::Swizzle<Math::Swizzle::kZZZZ>(lhs.m_rows[3]) * rhs.m_rows[2]
            + Vector4F::Swizzle<Math::Swizzle::kYYYY>(lhs.m_rows[3]) * rhs.m_rows[1]
            + Vector4F::Swizzle<Math::Swizzle::kXXXX>(lhs.m_rows[3]) * rhs.m_rows[0];

        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL operator*(const Vector4F lhs, const Matrix4x4F& rhs)
    {
        return Vector4F::Swizzle<Math::Swizzle::kWWWW>(lhs) * rhs.m_rows[3]
            + Vector4F::Swizzle<Math::Swizzle::kZZZZ>(lhs) * rhs.m_rows[2]
            + Vector4F::Swizzle<Math::Swizzle::kYYYY>(lhs) * rhs.m_rows[1]
            + Vector4F::Swizzle<Math::Swizzle::kXXXX>(lhs) * rhs.m_rows[0];
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4F FE_VECTORCALL operator*(const Matrix4x4F& lhs, const float rhs)
    {
        const __m128 broadcast = _mm_set1_ps(rhs);

        Matrix4x4F result;
        result.m_simdVectors[0] = _mm_mul_ps(lhs.m_simdVectors[0], broadcast);
        result.m_simdVectors[1] = _mm_mul_ps(lhs.m_simdVectors[1], broadcast);
        result.m_simdVectors[2] = _mm_mul_ps(lhs.m_simdVectors[2], broadcast);
        result.m_simdVectors[3] = _mm_mul_ps(lhs.m_simdVectors[3], broadcast);
        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4F FE_VECTORCALL operator/(const Matrix4x4F& lhs, const float rhs)
    {
        const __m128 broadcast = _mm_set1_ps(rhs);

        Matrix4x4F result;
        result.m_simdVectors[0] = _mm_div_ps(lhs.m_simdVectors[0], broadcast);
        result.m_simdVectors[1] = _mm_div_ps(lhs.m_simdVectors[1], broadcast);
        result.m_simdVectors[2] = _mm_div_ps(lhs.m_simdVectors[2], broadcast);
        result.m_simdVectors[3] = _mm_div_ps(lhs.m_simdVectors[3], broadcast);
        return result;
    }


    namespace Math
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix4x4F FE_VECTORCALL Transpose(const Matrix4x4F& matrix)
        {
            Matrix4x4F result;
            FE::Internal::Transpose4Impl(matrix.m_simdVectors[0],
                                         matrix.m_simdVectors[1],
                                         matrix.m_simdVectors[2],
                                         matrix.m_simdVectors[3],
                                         result.m_simdVectors);
            return result;
        }


        FE_NO_SECURITY_COOKIE Matrix4x4F FE_VECTORCALL InverseTransform(const Matrix4x4F& matrix);


        template<uint32_t TColumnIndex>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector4F FE_VECTORCALL ExtractColumn(const Matrix4x4F& matrix)
        {
            constexpr int32_t shuffleMask = _MM_SHUFFLE(TColumnIndex, TColumnIndex, TColumnIndex, TColumnIndex);
            const __m128 x = _mm_shuffle_ps(matrix.m_simdVectors[0], matrix.m_simdVectors[0], shuffleMask);
            const __m128 y = _mm_shuffle_ps(matrix.m_simdVectors[1], matrix.m_simdVectors[1], shuffleMask);
            const __m128 z = _mm_shuffle_ps(matrix.m_simdVectors[2], matrix.m_simdVectors[2], shuffleMask);
            const __m128 w = _mm_shuffle_ps(matrix.m_simdVectors[3], matrix.m_simdVectors[3], shuffleMask);
            return Vector4F{ _mm_blend_ps(_mm_blend_ps(_mm_blend_ps(x, y, 0x2), z, 0xc), w, 0x8) };
        }


        template<uint32_t TColumnIndex>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE void FE_VECTORCALL ReplaceColumn(Matrix4x4F& matrix, const Vector4F column)
        {
            constexpr uint32_t sourceMask = TColumnIndex << 4u;
            matrix.m_simdVectors[0] = _mm_insert_ps(matrix.m_simdVectors[0], column.m_simdVector, sourceMask | (0 << 6));
            matrix.m_simdVectors[1] = _mm_insert_ps(matrix.m_simdVectors[1], column.m_simdVector, sourceMask | (1 << 6));
            matrix.m_simdVectors[2] = _mm_insert_ps(matrix.m_simdVectors[2], column.m_simdVector, sourceMask | (2 << 6));
            matrix.m_simdVectors[3] = _mm_insert_ps(matrix.m_simdVectors[3], column.m_simdVector, sourceMask | (3 << 6));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EqualEstimate(Matrix4x4F lhs, Matrix4x4F rhs,
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
