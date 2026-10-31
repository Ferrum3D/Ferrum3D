#pragma once
#include <Core/Math/Matrix4x4.h>

namespace FE
{
    //! @brief Affine transform with an implicit (0, 0, 0, 1) column, 16 bytes smaller than Matrix4x4.
    //!
    //! Rows store the transposed upper 4x3 block of the equivalent row-vector Matrix4x4: m_rows[i] holds column i, so
    //! output coordinate i of a point is Dot(m_rows[i], Vector4(point, 1)). The layout matches a row-major HLSL float3x4
    //! used as mul(matrix, float4(point, 1)). Operators follow the Matrix4x4 convention: lhs * rhs applies lhs first.
    struct Matrix3x4 final
    {
        union
        {
            Vector4 m_rows[3];
            __m128 m_simdVectors[3];
            float m_values[12];
        };

        FE_RTTI_Reflect("818205F6-ECFE-424A-B455-DD4648F8ACD6");
        static Serialization::ResultCode Serialize(Serialization::SerializationContext& context, const Matrix3x4& value);
        static Serialization::ResultCode Deserialize(Serialization::DeserializationContext& context, Matrix3x4& value);

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix3x4() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix3x4(ForceInitType)
            : m_simdVectors{ _mm_setzero_ps(), _mm_setzero_ps(), _mm_setzero_ps() }
        {
        }

        //! @brief Drop the last column of an affine row-vector matrix.
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix3x4 FE_VECTORCALL FromMatrix4x4(const Matrix4x4& matrix)
        {
            __m128 columns[4];
            Internal::Transpose4Impl(matrix.m_simdVectors[0],
                                     matrix.m_simdVectors[1],
                                     matrix.m_simdVectors[2],
                                     matrix.m_simdVectors[3],
                                     columns);

            Matrix3x4 result;
            result.m_simdVectors[0] = columns[0];
            result.m_simdVectors[1] = columns[1];
            result.m_simdVectors[2] = columns[2];
            return result;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Matrix4x4 FE_VECTORCALL ToMatrix4x4(const Matrix3x4& matrix)
        {
            Matrix4x4 result;
            Internal::Transpose4Impl(matrix.m_simdVectors[0],
                                     matrix.m_simdVectors[1],
                                     matrix.m_simdVectors[2],
                                     _mm_load_ps(&Internal::kIdentity4Values[12]),
                                     result.m_simdVectors);
            return result;
        }

        [[nodiscard]] FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3 FE_VECTORCALL GetTranslation() const
        {
            return Vector3{ m_rows[0].w, m_rows[1].w, m_rows[2].w };
        }

        static const Matrix3x4 kIdentity;
    };


    //! @brief Compose two affine transforms; the result applies lhs first, as with Matrix4x4.
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Matrix3x4 FE_VECTORCALL operator*(const Matrix3x4& lhs, const Matrix3x4& rhs)
    {
        const __m128 axisW = _mm_load_ps(&Internal::kIdentity4Values[12]);

        Matrix3x4 result;
        for (uint32_t rowIndex = 0; rowIndex < 3; ++rowIndex)
        {
            const float* row = reinterpret_cast<const float*>(&rhs.m_simdVectors[rowIndex]);
            const __m128 x = _mm_mul_ps(_mm_broadcast_ss(row + 0), lhs.m_simdVectors[0]);
            const __m128 y = _mm_mul_ps(_mm_broadcast_ss(row + 1), lhs.m_simdVectors[1]);
            const __m128 z = _mm_mul_ps(_mm_broadcast_ss(row + 2), lhs.m_simdVectors[2]);
            const __m128 w = _mm_mul_ps(_mm_broadcast_ss(row + 3), axisW);
            result.m_simdVectors[rowIndex] = _mm_add_ps(_mm_add_ps(x, y), _mm_add_ps(z, w));
        }

        return result;
    }


    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator==(const Matrix3x4& lhs, const Matrix3x4& rhs)
    {
        return lhs.m_rows[0] == rhs.m_rows[0] && lhs.m_rows[1] == rhs.m_rows[1] && lhs.m_rows[2] == rhs.m_rows[2];
    }


    namespace Math
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3 FE_VECTORCALL TransformPoint(const Matrix3x4& matrix, const Vector3 point)
        {
            const Vector4 homogeneous(point, 1.0f);
            return Vector3{ Dot(matrix.m_rows[0], homogeneous),
                            Dot(matrix.m_rows[1], homogeneous),
                            Dot(matrix.m_rows[2], homogeneous) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL CmpEqual(const Matrix3x4& lhs, const Matrix3x4& rhs,
                                                                          const float epsilon = Constants::kEpsilon)
        {
            return CmpEqual(lhs.m_rows[0], rhs.m_rows[0], epsilon) && CmpEqual(lhs.m_rows[1], rhs.m_rows[1], epsilon)
                && CmpEqual(lhs.m_rows[2], rhs.m_rows[2], epsilon);
        }
    } // namespace Math
} // namespace FE
