#include <FeCore/Math/Matrix4x4F.h>

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


    namespace Math
    {
        Matrix4x4F FE_VECTORCALL InverseTransform(const Matrix4x4F& matrix)
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

            Matrix4x4F m;
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
    } // namespace Math
} // namespace FE
