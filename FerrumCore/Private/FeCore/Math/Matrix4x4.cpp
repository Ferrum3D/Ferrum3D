#include <FeCore/Math/Matrix4x4.h>

namespace FE
{
    alignas(Matrix4x4) const float Internal::kIdentity4Values[16] = {
        1.0f, 0.0f, 0.0f, 0.0f, //
        0.0f, 1.0f, 0.0f, 0.0f, //
        0.0f, 0.0f, 1.0f, 0.0f, //
        0.0f, 0.0f, 0.0f, 1.0f, //
    };

    const Matrix4x4 Matrix4x4::kIdentity = LoadAligned(&Internal::kIdentity4Values[0]);


    Matrix4x4 FE_VECTORCALL Math::InvertTransform(const Matrix4x4& matrix)
    {
        const float s0 = matrix.m_00 * matrix.m_11 - matrix.m_10 * matrix.m_01;
        const float s1 = matrix.m_00 * matrix.m_12 - matrix.m_10 * matrix.m_02;
        const float s2 = matrix.m_00 * matrix.m_13 - matrix.m_10 * matrix.m_03;
        const float s3 = matrix.m_01 * matrix.m_12 - matrix.m_11 * matrix.m_02;
        const float s4 = matrix.m_01 * matrix.m_13 - matrix.m_11 * matrix.m_03;
        const float s5 = matrix.m_02 * matrix.m_13 - matrix.m_12 * matrix.m_03;

        const float c5 = matrix.m_22 * matrix.m_33 - matrix.m_32 * matrix.m_23;
        const float c4 = matrix.m_21 * matrix.m_33 - matrix.m_31 * matrix.m_23;
        const float c3 = matrix.m_21 * matrix.m_32 - matrix.m_31 * matrix.m_22;
        const float c2 = matrix.m_20 * matrix.m_33 - matrix.m_30 * matrix.m_23;
        const float c1 = matrix.m_20 * matrix.m_32 - matrix.m_30 * matrix.m_22;
        const float c0 = matrix.m_20 * matrix.m_31 - matrix.m_30 * matrix.m_21;

        const float invDet = 1.0f / (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);

        Matrix4x4 m;
        m.m_00 = (matrix.m_11 * c5 - matrix.m_12 * c4 + matrix.m_13 * c3) * invDet;
        m.m_01 = (-matrix.m_01 * c5 + matrix.m_02 * c4 - matrix.m_03 * c3) * invDet;
        m.m_02 = (matrix.m_31 * s5 - matrix.m_32 * s4 + matrix.m_33 * s3) * invDet;
        m.m_03 = (-matrix.m_21 * s5 + matrix.m_22 * s4 - matrix.m_23 * s3) * invDet;

        m.m_10 = (-matrix.m_10 * c5 + matrix.m_12 * c2 - matrix.m_13 * c1) * invDet;
        m.m_11 = (matrix.m_00 * c5 - matrix.m_02 * c2 + matrix.m_03 * c1) * invDet;
        m.m_12 = (-matrix.m_30 * s5 + matrix.m_32 * s2 - matrix.m_33 * s1) * invDet;
        m.m_13 = (matrix.m_20 * s5 - matrix.m_22 * s2 + matrix.m_23 * s1) * invDet;

        m.m_20 = (matrix.m_10 * c4 - matrix.m_11 * c2 + matrix.m_13 * c0) * invDet;
        m.m_21 = (-matrix.m_00 * c4 + matrix.m_01 * c2 - matrix.m_03 * c0) * invDet;
        m.m_22 = (matrix.m_30 * s4 - matrix.m_31 * s2 + matrix.m_33 * s0) * invDet;
        m.m_23 = (-matrix.m_20 * s4 + matrix.m_21 * s2 - matrix.m_23 * s0) * invDet;

        m.m_30 = (-matrix.m_10 * c3 + matrix.m_11 * c1 - matrix.m_12 * c0) * invDet;
        m.m_31 = (matrix.m_00 * c3 - matrix.m_01 * c1 + matrix.m_02 * c0) * invDet;
        m.m_32 = (-matrix.m_30 * s3 + matrix.m_31 * s1 - matrix.m_32 * s0) * invDet;
        m.m_33 = (matrix.m_20 * s3 - matrix.m_21 * s1 + matrix.m_22 * s0) * invDet;

        return m;
    }


    Matrix4x4 FE_VECTORCALL Math::Invert(const Matrix4x4& matrix)
    {
        Matrix4x4 MT = Transpose(matrix);
        __m128 V00 = _mm_shuffle_ps(MT.m_rows[2].m_simdVector, MT.m_rows[2].m_simdVector, _MM_SHUFFLE(1, 1, 0, 0));
        __m128 V10 = _mm_shuffle_ps(MT.m_rows[3].m_simdVector, MT.m_rows[3].m_simdVector, _MM_SHUFFLE(3, 2, 3, 2));
        __m128 V01 = _mm_shuffle_ps(MT.m_rows[0].m_simdVector, MT.m_rows[0].m_simdVector, _MM_SHUFFLE(1, 1, 0, 0));
        __m128 V11 = _mm_shuffle_ps(MT.m_rows[1].m_simdVector, MT.m_rows[1].m_simdVector, _MM_SHUFFLE(3, 2, 3, 2));
        __m128 V02 = _mm_shuffle_ps(MT.m_rows[2].m_simdVector, MT.m_rows[0].m_simdVector, _MM_SHUFFLE(2, 0, 2, 0));
        __m128 V12 = _mm_shuffle_ps(MT.m_rows[3].m_simdVector, MT.m_rows[1].m_simdVector, _MM_SHUFFLE(3, 1, 3, 1));

        __m128 D0 = _mm_mul_ps(V00, V10);
        __m128 D1 = _mm_mul_ps(V01, V11);
        __m128 D2 = _mm_mul_ps(V02, V12);

        V00 = _mm_shuffle_ps(MT.m_rows[2].m_simdVector, MT.m_rows[2].m_simdVector, _MM_SHUFFLE(3, 2, 3, 2));
        V10 = _mm_shuffle_ps(MT.m_rows[3].m_simdVector, MT.m_rows[3].m_simdVector, _MM_SHUFFLE(1, 1, 0, 0));
        V01 = _mm_shuffle_ps(MT.m_rows[0].m_simdVector, MT.m_rows[0].m_simdVector, _MM_SHUFFLE(3, 2, 3, 2));
        V11 = _mm_shuffle_ps(MT.m_rows[1].m_simdVector, MT.m_rows[1].m_simdVector, _MM_SHUFFLE(1, 1, 0, 0));
        V02 = _mm_shuffle_ps(MT.m_rows[2].m_simdVector, MT.m_rows[0].m_simdVector, _MM_SHUFFLE(3, 1, 3, 1));
        V12 = _mm_shuffle_ps(MT.m_rows[3].m_simdVector, MT.m_rows[1].m_simdVector, _MM_SHUFFLE(2, 0, 2, 0));

        V00 = _mm_mul_ps(V00, V10);
        V01 = _mm_mul_ps(V01, V11);
        V02 = _mm_mul_ps(V02, V12);
        D0 = _mm_sub_ps(D0, V00);
        D1 = _mm_sub_ps(D1, V01);
        D2 = _mm_sub_ps(D2, V02);
        // V11 = D0Y,D0W,D2Y,D2Y
        V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 1, 3, 1));
        V00 = _mm_shuffle_ps(MT.m_rows[1].m_simdVector, MT.m_rows[1].m_simdVector, _MM_SHUFFLE(1, 0, 2, 1));
        V10 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(0, 3, 0, 2));
        V01 = _mm_shuffle_ps(MT.m_rows[0].m_simdVector, MT.m_rows[0].m_simdVector, _MM_SHUFFLE(0, 1, 0, 2));
        V11 = _mm_shuffle_ps(V11, D0, _MM_SHUFFLE(2, 1, 2, 1));
        // V13 = D1Y,D1W,D2W,D2W
        __m128 V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 3, 3, 1));
        V02 = _mm_shuffle_ps(MT.m_rows[3].m_simdVector, MT.m_rows[3].m_simdVector, _MM_SHUFFLE(1, 0, 2, 1));
        V12 = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(0, 3, 0, 2));
        __m128 V03 = _mm_shuffle_ps(MT.m_rows[2].m_simdVector, MT.m_rows[2].m_simdVector, _MM_SHUFFLE(0, 1, 0, 2));
        V13 = _mm_shuffle_ps(V13, D1, _MM_SHUFFLE(2, 1, 2, 1));

        __m128 C0 = _mm_mul_ps(V00, V10);
        __m128 C2 = _mm_mul_ps(V01, V11);
        __m128 C4 = _mm_mul_ps(V02, V12);
        __m128 C6 = _mm_mul_ps(V03, V13);

        // V11 = D0X,D0Y,D2X,D2X
        V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(0, 0, 1, 0));
        V00 = _mm_shuffle_ps(MT.m_rows[1].m_simdVector, MT.m_rows[1].m_simdVector, _MM_SHUFFLE(2, 1, 3, 2));
        V10 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(2, 1, 0, 3));
        V01 = _mm_shuffle_ps(MT.m_rows[0].m_simdVector, MT.m_rows[0].m_simdVector, _MM_SHUFFLE(1, 3, 2, 3));
        V11 = _mm_shuffle_ps(D0, V11, _MM_SHUFFLE(0, 2, 1, 2));
        // V13 = D1X,D1Y,D2Z,D2Z
        V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(2, 2, 1, 0));
        V02 = _mm_shuffle_ps(MT.m_rows[3].m_simdVector, MT.m_rows[3].m_simdVector, _MM_SHUFFLE(2, 1, 3, 2));
        V12 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(2, 1, 0, 3));
        V03 = _mm_shuffle_ps(MT.m_rows[2].m_simdVector, MT.m_rows[2].m_simdVector, _MM_SHUFFLE(1, 3, 2, 3));
        V13 = _mm_shuffle_ps(D1, V13, _MM_SHUFFLE(0, 2, 1, 2));

        V00 = _mm_mul_ps(V00, V10);
        V01 = _mm_mul_ps(V01, V11);
        V02 = _mm_mul_ps(V02, V12);
        V03 = _mm_mul_ps(V03, V13);
        C0 = _mm_sub_ps(C0, V00);
        C2 = _mm_sub_ps(C2, V01);
        C4 = _mm_sub_ps(C4, V02);
        C6 = _mm_sub_ps(C6, V03);

        V00 = _mm_shuffle_ps(MT.m_rows[1].m_simdVector, MT.m_rows[1].m_simdVector, _MM_SHUFFLE(0, 3, 0, 3));
        // V10 = D0Z,D0Z,D2X,D2Y
        V10 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 2, 2));
        V10 = _mm_shuffle_ps(V10, V10, _MM_SHUFFLE(0, 2, 3, 0));
        V01 = _mm_shuffle_ps(MT.m_rows[0].m_simdVector, MT.m_rows[0].m_simdVector, _MM_SHUFFLE(2, 0, 3, 1));
        // V11 = D0X,D0W,D2X,D2Y
        V11 = _mm_shuffle_ps(D0, D2, _MM_SHUFFLE(1, 0, 3, 0));
        V11 = _mm_shuffle_ps(V11, V11, _MM_SHUFFLE(2, 1, 0, 3));
        V02 = _mm_shuffle_ps(MT.m_rows[3].m_simdVector, MT.m_rows[3].m_simdVector, _MM_SHUFFLE(0, 3, 0, 3));
        // V12 = D1Z,D1Z,D2Z,D2W
        V12 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 2, 2));
        V12 = _mm_shuffle_ps(V12, V12, _MM_SHUFFLE(0, 2, 3, 0));
        V03 = _mm_shuffle_ps(MT.m_rows[2].m_simdVector, MT.m_rows[2].m_simdVector, _MM_SHUFFLE(2, 0, 3, 1));
        // V13 = D1X,D1W,D2Z,D2W
        V13 = _mm_shuffle_ps(D1, D2, _MM_SHUFFLE(3, 2, 3, 0));
        V13 = _mm_shuffle_ps(V13, V13, _MM_SHUFFLE(2, 1, 0, 3));

        V00 = _mm_mul_ps(V00, V10);
        V01 = _mm_mul_ps(V01, V11);
        V02 = _mm_mul_ps(V02, V12);
        V03 = _mm_mul_ps(V03, V13);
        __m128 C1 = _mm_sub_ps(C0, V00);
        C0 = _mm_add_ps(C0, V00);
        __m128 C3 = _mm_add_ps(C2, V01);
        C2 = _mm_sub_ps(C2, V01);
        __m128 C5 = _mm_sub_ps(C4, V02);
        C4 = _mm_add_ps(C4, V02);
        __m128 C7 = _mm_add_ps(C6, V03);
        C6 = _mm_sub_ps(C6, V03);

        C0 = _mm_shuffle_ps(C0, C1, _MM_SHUFFLE(3, 1, 2, 0));
        C2 = _mm_shuffle_ps(C2, C3, _MM_SHUFFLE(3, 1, 2, 0));
        C4 = _mm_shuffle_ps(C4, C5, _MM_SHUFFLE(3, 1, 2, 0));
        C6 = _mm_shuffle_ps(C6, C7, _MM_SHUFFLE(3, 1, 2, 0));
        C0 = _mm_shuffle_ps(C0, C0, _MM_SHUFFLE(3, 1, 2, 0));
        C2 = _mm_shuffle_ps(C2, C2, _MM_SHUFFLE(3, 1, 2, 0));
        C4 = _mm_shuffle_ps(C4, C4, _MM_SHUFFLE(3, 1, 2, 0));
        C6 = _mm_shuffle_ps(C6, C6, _MM_SHUFFLE(3, 1, 2, 0));

        __m128 vTemp = Simd::DotProduct(C0, MT.m_rows[0].m_simdVector);
        vTemp = _mm_div_ps(Simd::SSE::Constants::kAllOnes, vTemp);

        Matrix4x4 m;
        m.m_rows[0].m_simdVector = _mm_mul_ps(C0, vTemp);
        m.m_rows[1].m_simdVector = _mm_mul_ps(C2, vTemp);
        m.m_rows[2].m_simdVector = _mm_mul_ps(C4, vTemp);
        m.m_rows[3].m_simdVector = _mm_mul_ps(C6, vTemp);
        return m;
    }


    bool FE_VECTORCALL Math::DecomposeTransform(const Matrix4x4& matrix, Vector3& translation, Quaternion& rotation,
                                                Vector3& scale, Vector3& shear)
    {
        //
        // We are using the method described in Graphics Gems II,
        // VII.1 "Decomposing a Matrix into Simple Transformations"
        //

        Vector3 m[4];
        _mm_store_ps(&m[0].x, matrix.m_simdVectors[0]);
        _mm_store_ps(&m[1].x, matrix.m_simdVectors[1]);
        _mm_store_ps(&m[2].x, matrix.m_simdVectors[2]);
        _mm_store_ps(&m[3].x, matrix.m_simdVectors[3]);
        translation = m[3];

        const auto checkedDiv = [](Vector3& v, const float d) {
            const float absD = Abs(d);
            if (absD < 1.0f && CmpGreaterEqualMask(Abs(v), Vector3(absD * Constants::kMaxFloat)) != 0)
                return false;

            v = v / d;
            return true;
        };

        scale.x = Length(m[0]);
        if (!checkedDiv(m[0], scale.x))
            return false;

        shear.x = Dot(m[0], m[1]);
        m[1] = m[1] - m[0] * shear.x;

        scale.y = Length(m[1]);
        if (!checkedDiv(m[1], scale.y))
            return false;

        shear.x /= scale.y;

        shear.y = Dot(m[0], m[2]);
        m[2] = m[2] - m[0] * shear.y;
        shear.z = Dot(m[1], m[2]);
        m[2] = m[2] - m[1] * shear.z;

        scale.z = Length(m[2]);
        if (!checkedDiv(m[2], scale.z))
            return false;

        shear.y /= scale.z;
        shear.z /= scale.z;

        const float det = Dot(m[0], Cross(m[1], m[2]));
        if (det < 0.0f)
        {
            scale = -scale;
            m[0] = -m[0];
            m[1] = -m[1];
            m[2] = -m[2];
        }

        rotation = ExtractRotation(Matrix4x4::LoadAligned(&m[0].x));
        return true;
    }


    Quaternion FE_VECTORCALL Math::ExtractRotation(const Matrix4x4& matrix)
    {
        //
        // From rtm: quat_from_matrix
        //

        const float xx = matrix.m_rows[0].x;
        const float yy = matrix.m_rows[1].y;
        const float zz = matrix.m_rows[2].z;

        const float trace = xx + yy + zz;
        if (trace > 0.0f)
        {
            const float xy = matrix.m_rows[0].y;
            const float xz = matrix.m_rows[0].z;

            const float yx = matrix.m_rows[1].x;
            const float yz = matrix.m_rows[1].z;

            const float zx = matrix.m_rows[2].x;
            const float zy = matrix.m_rows[2].y;

            const float invTrace = ReciprocalSqrt(trace + 1.0f);
            const float halfInvTrace = 0.5f * invTrace;

            return Quaternion{
                (yz - zy) * halfInvTrace,
                (zx - xz) * halfInvTrace,
                (xy - yx) * halfInvTrace,
                Reciprocal(invTrace) * 0.5f,
            };
        }

        // Find the axis with the highest diagonal value
        int32_t axis0 = 0;
        if (yy > xx)
            axis0 = 1;

        if (zz > matrix[axis0][axis0])
            axis0 = 2;

        const int32_t axis1 = (axis0 + 1) % 3;
        const int32_t axis2 = (axis1 + 1) % 3;

        const float pseudoTrace = 1.0f + matrix[axis0][axis0] - matrix[axis1][axis1] - matrix[axis2][axis2];
        const float inversePseudoTrace = ReciprocalSqrt(pseudoTrace);
        const float halfInversePseudoTrace = inversePseudoTrace * 0.5f;

        float quat[4];
        quat[axis0] = Reciprocal(inversePseudoTrace) * 0.5f;
        quat[axis1] = halfInversePseudoTrace * (matrix[axis0][axis1] + matrix[axis1][axis0]);
        quat[axis2] = halfInversePseudoTrace * (matrix[axis0][axis2] + matrix[axis2][axis0]);
        quat[3] = halfInversePseudoTrace * (matrix[axis1][axis2] - matrix[axis2][axis1]);
        return Normalize(Quaternion::LoadUnaligned(quat));
    }
} // namespace FE
