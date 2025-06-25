#include <FeCore/Math/Matrix4x4.h>

namespace FE
{
    namespace Internal
    {
        // We align this array to the CPU cache line size since the entire matrix fits there.
        // This helps to reduce cache misses when loading identity matrices (supposedly).
        alignas(Memory::kCacheLineSize) const float kIdentity4Values[16] = {
            1.0f, 0.0f, 0.0f, 0.0f, //
            0.0f, 1.0f, 0.0f, 0.0f, //
            0.0f, 0.0f, 1.0f, 0.0f, //
            0.0f, 0.0f, 0.0f, 1.0f, //
        };
    } // namespace Internal


    Matrix4x4 FE_VECTORCALL Math::InverseTransform(const Matrix4x4& matrix)
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
