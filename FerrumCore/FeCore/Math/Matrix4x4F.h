#pragma once
#include <FeCore/Math/Vector4.h>

namespace FE
{
    class Matrix4x4F
    {
        inline static constexpr size_t RowCount = 4;

        Vector4F m_Data[RowCount];

    public:
        FE_STRUCT_RTTI(Matrix4x4F, "F86BB569-A2F4-48B7-83BE-365D28E862BD");

        FE_FINLINE Matrix4x4F() = default;

        FE_FINLINE Matrix4x4F(const Matrix4x4F& other) noexcept;

        [[nodiscard]] FE_FINLINE static Matrix4x4F GetZero() noexcept;
        [[nodiscard]] FE_FINLINE static Matrix4x4F GetIdentity() noexcept;

        [[nodiscard]] FE_FINLINE static Matrix4x4F FromRows(
            const Vector4F& row0, const Vector4F& row1, const Vector4F& row2, const Vector4F& row3);
        [[nodiscard]] FE_FINLINE static Matrix4x4F FromColumns(
            const Vector4F& column0, const Vector4F& column1, const Vector4F& column2, const Vector4F& column3);

        [[nodiscard]] FE_FINLINE static Matrix4x4F CreateRotationX(Float32 angle);
        [[nodiscard]] FE_FINLINE static Matrix4x4F CreateRotationY(Float32 angle);
        [[nodiscard]] FE_FINLINE static Matrix4x4F CreateRotationZ(Float32 angle);

        [[nodiscard]] FE_FINLINE static Matrix4x4F CreateScale(const Vector3F& scale);
        [[nodiscard]] FE_FINLINE static Matrix4x4F CreateDiagonal(const Vector4F& diagonal);
        [[nodiscard]] FE_FINLINE static Matrix4x4F CreateDiagonal(Float32 x, Float32 y, Float32 z, Float32 w);

        [[nodiscard]] FE_FINLINE static Matrix4x4F CreateTranslation(const Vector3F& translation);

        // Vulkan-compatible
        [[nodiscard]] FE_FINLINE static Matrix4x4F CreateProjection(Float32 fovY, Float32 aspect, Float32 near, Float32 far);

        [[nodiscard]] FE_FINLINE const Float32* RowMajorData() const;

        [[nodiscard]] FE_FINLINE Float32 operator()(size_t row, size_t column) const;
        [[nodiscard]] FE_FINLINE Float32& operator()(size_t row, size_t column);

        [[nodiscard]] FE_FINLINE Vector4F GetRow(size_t index) const;
        FE_FINLINE void SetRow(size_t index, const Vector4F& vector);
        FE_FINLINE void SetRow(size_t index, Float32 x, Float32 y, Float32 z, Float32 w);
        FE_FINLINE void SetRow(size_t index, const Vector3F& vector, Float32 w = 1.0f);

        [[nodiscard]] FE_FINLINE Vector4F GetColumn(size_t index) const;
        FE_FINLINE void SetColumn(size_t index, const Vector4F& vector);
        FE_FINLINE void SetColumn(size_t index, Float32 x, Float32 y, Float32 z, Float32 w);
        FE_FINLINE void SetColumn(size_t index, const Vector3F& vector, Float32 w = 1.0f);

        [[nodiscard]] FE_FINLINE Vector4F GetBasisX() const;
        [[nodiscard]] FE_FINLINE Vector4F GetBasisY() const;
        [[nodiscard]] FE_FINLINE Vector4F GetBasisZ() const;

        [[nodiscard]] FE_FINLINE Matrix4x4F operator+(const Matrix4x4F& other) const;
        [[nodiscard]] FE_FINLINE Matrix4x4F operator-(const Matrix4x4F& other) const;
        [[nodiscard]] FE_FINLINE Matrix4x4F operator*(const Matrix4x4F& other) const;
        [[nodiscard]] FE_FINLINE Matrix4x4F operator*(Float32 f) const;
        [[nodiscard]] FE_FINLINE Matrix4x4F operator/(Float32 f) const;

        FE_FINLINE Matrix4x4F& operator+=(const Matrix4x4F& other);
        FE_FINLINE Matrix4x4F& operator-=(const Matrix4x4F& other);
        FE_FINLINE Matrix4x4F& operator*=(const Matrix4x4F& other);
        FE_FINLINE Matrix4x4F& operator*=(Float32 f);
        FE_FINLINE Matrix4x4F& operator/=(Float32 f);

        [[nodiscard]] FE_FINLINE Matrix4x4F operator-() const;

        [[nodiscard]] FE_FINLINE Vector3F operator*(const Vector3F& vector) const;
        [[nodiscard]] FE_FINLINE Vector4F operator*(const Vector4F& vector) const;

        [[nodiscard]] FE_FINLINE Matrix4x4F Transposed() const;
        FE_FINLINE void Transpose();

        [[nodiscard]] FE_FINLINE Matrix4x4F InverseTransform() const;
        FE_FINLINE void InvertTransform();

        [[nodiscard]] FE_FINLINE Float32 Determinant() const;

        [[nodiscard]] FE_FINLINE bool IsApproxEqualTo(
            const Matrix4x4F& other, Float32 epsilon = Constants::Epsilon) const noexcept;

        [[nodiscard]] FE_FINLINE bool operator==(const Matrix4x4F& other) const noexcept;
        [[nodiscard]] FE_FINLINE bool operator!=(const Matrix4x4F& other) const noexcept;
    };

    namespace Internal
    {
        FE_FINLINE void SIMDMatrix4x4Multiply(
            const SIMD::SSE::Float32x4* l, const SIMD::SSE::Float32x4* r, SIMD::SSE::Float32x4* out)
        {
            // clang-format off
            out[0] = l[0].Broadcast<3>() * r[3] + (l[0].Broadcast<2>() * r[2] + (l[0].Broadcast<1>() * r[1] + (l[0].Broadcast<0>() * r[0])));
            out[1] = l[1].Broadcast<3>() * r[3] + (l[1].Broadcast<2>() * r[2] + (l[1].Broadcast<1>() * r[1] + (l[1].Broadcast<0>() * r[0])));
            out[2] = l[2].Broadcast<3>() * r[3] + (l[2].Broadcast<2>() * r[2] + (l[2].Broadcast<1>() * r[1] + (l[2].Broadcast<0>() * r[0])));
            out[3] = l[3].Broadcast<3>() * r[3] + (l[3].Broadcast<2>() * r[2] + (l[3].Broadcast<1>() * r[1] + (l[3].Broadcast<0>() * r[0])));
            // clang-format on
        }

        FE_FINLINE SIMD::SSE::Float32x4 SIMDMatrix4x4VectorMultiply(
            const SIMD::SSE::Float32x4* matrix, SIMD::SSE::Float32x4 vector)
        {
            using namespace SIMD::SSE;
            Float32x4 prod1 = matrix[0] * vector;
            Float32x4 prod2 = matrix[1] * vector;
            Float32x4 prod3 = matrix[2] * vector;
            Float32x4 prod4 = matrix[3] * vector;

            return prod1.HorizontalAdd(prod2).HorizontalAdd(prod3.HorizontalAdd(prod4));
        }

        FE_FINLINE Float32 SIMDMatrix3x3Determinant(const Vector3F* matrix)
        {
            // Cross and Dot are already SIMD-optimized
            //                  | a1 a2 a3 |
            // a * [b x c] = det| b1 b2 b3 |
            //                  | c1 c2 c3 |
            return matrix[0].Dot(matrix[1].Cross(matrix[2]));
        }
    } // namespace Internal

    Matrix4x4F::Matrix4x4F(const Matrix4x4F& other) noexcept
    {
        m_Data[0] = other.m_Data[0];
        m_Data[1] = other.m_Data[1];
        m_Data[2] = other.m_Data[2];
        m_Data[3] = other.m_Data[3];
    }

    Matrix4x4F Matrix4x4F::GetZero() noexcept
    {
        return Matrix4x4F::FromRows(Vector4F::GetZero(), Vector4F::GetZero(), Vector4F::GetZero(), Vector4F::GetZero());
    }

    Matrix4x4F Matrix4x4F::GetIdentity() noexcept
    {
        return Matrix4x4F::FromRows(Vector4F::GetUnitX(), Vector4F::GetUnitY(), Vector4F::GetUnitZ(), Vector4F::GetUnitW());
    }

    Matrix4x4F Matrix4x4F::FromRows(const Vector4F& row0, const Vector4F& row1, const Vector4F& row2, const Vector4F& row3)
    {
        Matrix4x4F matrix{};
        matrix.SetRow(0, row0);
        matrix.SetRow(1, row1);
        matrix.SetRow(2, row2);
        matrix.SetRow(3, row3);
        return matrix;
    }

    Matrix4x4F Matrix4x4F::FromColumns(
        const Vector4F& column0, const Vector4F& column1, const Vector4F& column2, const Vector4F& column3)
    {
        Matrix4x4F matrix{};
        matrix.SetColumn(0, column0);
        matrix.SetColumn(1, column1);
        matrix.SetColumn(2, column2);
        matrix.SetColumn(3, column3);
        return matrix;
    }

    Matrix4x4F Matrix4x4F::CreateRotationX(Float32 angle)
    {
        Float32 s = std::sin(angle);
        Float32 c = std::cos(angle);
        Matrix4x4F matrix{};
        matrix.SetRow(0, Vector4F::GetUnitX());
        matrix.SetRow(1, 0.0f, c, -s, 0.0f);
        matrix.SetRow(2, 0.0f, s, +c, 0.0f);
        matrix.SetRow(3, Vector4F::GetUnitW());
        return matrix;
    }

    Matrix4x4F Matrix4x4F::CreateRotationY(Float32 angle)
    {
        Float32 s = std::sin(angle);
        Float32 c = std::cos(angle);
        Matrix4x4F matrix{};
        matrix.SetRow(0, +c, 0.0f, s, 0.0f);
        matrix.SetRow(1, Vector4F::GetUnitY());
        matrix.SetRow(2, -s, 0.0f, c, 0.0f);
        matrix.SetRow(3, Vector4F::GetUnitW());
        return matrix;
    }

    Matrix4x4F Matrix4x4F::CreateRotationZ(Float32 angle)
    {
        Float32 s = std::sin(angle);
        Float32 c = std::cos(angle);
        Matrix4x4F matrix{};
        matrix.SetRow(0, c, -s, 0.0f, 0.0f);
        matrix.SetRow(1, s, +c, 0.0f, 0.0f);
        matrix.SetRow(2, Vector4F::GetUnitZ());
        matrix.SetRow(3, Vector4F::GetUnitW());
        return matrix;
    }

    Matrix4x4F Matrix4x4F::CreateScale(const Vector3F& scale)
    {
        return CreateDiagonal(Vector4F(scale));
    }

    Matrix4x4F Matrix4x4F::CreateDiagonal(const Vector4F& diagonal)
    {
        return CreateDiagonal(diagonal.X(), diagonal.Y(), diagonal.Z(), diagonal.W());
    }

    Matrix4x4F Matrix4x4F::CreateDiagonal(Float32 x, Float32 y, Float32 z, Float32 w)
    {
        Matrix4x4F matrix{};
        matrix.SetRow(0, x, 0.0f, 0.0f, 0.0f);
        matrix.SetRow(1, 0.0f, y, 0.0f, 0.0f);
        matrix.SetRow(2, 0.0f, 0.0f, z, 0.0f);
        matrix.SetRow(3, 0.0f, 0.0f, 0.0f, w);
        return matrix;
    }

    Matrix4x4F Matrix4x4F::CreateTranslation(const Vector3F& translation)
    {
        Matrix4x4F matrix{};
        matrix.SetRow(0, 1.0f, 0.0f, 0.0f, translation.X());
        matrix.SetRow(1, 0.0f, 1.0f, 0.0f, translation.Y());
        matrix.SetRow(2, 0.0f, 0.0f, 1.0f, translation.Z());
        matrix.SetRow(3, 0.0f, 0.0f, 0.0f, 1.0f);
        return matrix;
    }

    Matrix4x4F Matrix4x4F::CreateProjection(Float32 fovY, Float32 aspect, Float32 near, Float32 far)
    {
        Matrix4x4F matrix{};
        Float32 focalLength = 1.0f / std::tan(fovY / 2.0f);

        matrix.SetRow(0, focalLength / aspect, 0.0f, 0.0f, 0.0f);
        matrix.SetRow(1, 0.0f, -focalLength, 0.0f, 0.0f);
        matrix.SetRow(2, 0.0f, 0.0f, near / (far - near), far * near / (far - near));
        matrix.SetRow(3, 0.0f, 0.0f, -1.0f, 1.0f);
        return matrix;
    }

    const Float32* Matrix4x4F::RowMajorData() const
    {
        return m_Data[0].Data();
    }

    Float32 Matrix4x4F::operator()(size_t row, size_t column) const
    {
        return m_Data[row][column];
    }

    Float32& Matrix4x4F::operator()(size_t row, size_t column)
    {
        return m_Data[row](column);
    }

    Vector4F Matrix4x4F::GetRow(size_t index) const
    {
        return m_Data[index];
    }

    void Matrix4x4F::SetRow(size_t index, const Vector4F& vector)
    {
        m_Data[index] = vector;
    }

    void Matrix4x4F::SetRow(size_t index, Float32 x, Float32 y, Float32 z, Float32 w)
    {
        m_Data[index] = Vector4F(x, y, z, w);
    }

    void Matrix4x4F::SetRow(size_t index, const Vector3F& vector, Float32 w)
    {
        m_Data[index] = Vector4F(vector, w);
    }

    Vector4F Matrix4x4F::GetColumn(size_t index) const
    {
        return Vector4F(m_Data[0][index], m_Data[1][index], m_Data[2][index], m_Data[3][index]);
    }

    void Matrix4x4F::SetColumn(size_t index, const Vector4F& vector)
    {
        m_Data[0](index) = vector.X();
        m_Data[1](index) = vector.Y();
        m_Data[2](index) = vector.Z();
        m_Data[3](index) = vector.W();
    }

    void Matrix4x4F::SetColumn(size_t index, Float32 x, Float32 y, Float32 z, Float32 w)
    {
        m_Data[0](index) = x;
        m_Data[1](index) = y;
        m_Data[2](index) = z;
        m_Data[3](index) = w;
    }

    void Matrix4x4F::SetColumn(size_t index, const Vector3F& vector, Float32 w)
    {
        m_Data[0](index) = vector.X();
        m_Data[1](index) = vector.Y();
        m_Data[2](index) = vector.Z();
        m_Data[3](index) = w;
    }

    Vector4F Matrix4x4F::GetBasisX() const
    {
        return GetColumn(0);
    }

    Vector4F Matrix4x4F::GetBasisY() const
    {
        return GetColumn(1);
    }

    Vector4F Matrix4x4F::GetBasisZ() const
    {
        return GetColumn(2);
    }

    Matrix4x4F Matrix4x4F::operator+(const Matrix4x4F& other) const
    {
        auto row0 = GetRow(0) + other.GetRow(0);
        auto row1 = GetRow(1) + other.GetRow(1);
        auto row2 = GetRow(2) + other.GetRow(2);
        auto row3 = GetRow(3) + other.GetRow(3);
        return Matrix4x4F::FromRows(row0, row1, row2, row3);
    }

    Matrix4x4F Matrix4x4F::operator-(const Matrix4x4F& other) const
    {
        auto row0 = GetRow(0) - other.GetRow(0);
        auto row1 = GetRow(1) - other.GetRow(1);
        auto row2 = GetRow(2) - other.GetRow(2);
        auto row3 = GetRow(3) - other.GetRow(3);
        return Matrix4x4F::FromRows(row0, row1, row2, row3);
    }

    Matrix4x4F& Matrix4x4F::operator+=(const Matrix4x4F& other)
    {
        *this = *this + other;
        return *this;
    }

    Matrix4x4F& Matrix4x4F::operator-=(const Matrix4x4F& other)
    {
        *this = *this - other;
        return *this;
    }

    Matrix4x4F Matrix4x4F::operator*(const Matrix4x4F& other) const
    {
        Matrix4x4F matrix{};
        Internal::SIMDMatrix4x4Multiply(
            &m_Data[0].GetSIMD(), &other.m_Data[0].GetSIMD(), const_cast<SIMD::SSE::Float32x4*>(&matrix.m_Data[0].GetSIMD()));
        return matrix;
    }

    Matrix4x4F& Matrix4x4F::operator*=(const Matrix4x4F& other)
    {
        *this = *this * other;
        return *this;
    }

    Matrix4x4F Matrix4x4F::operator*(Float32 f) const
    {
        Matrix4x4F matrix{};
        matrix.SetRow(0, m_Data[0] * f);
        matrix.SetRow(1, m_Data[1] * f);
        matrix.SetRow(2, m_Data[2] * f);
        matrix.SetRow(3, m_Data[3] * f);
        return matrix;
    }

    Matrix4x4F Matrix4x4F::operator/(Float32 f) const
    {
        Matrix4x4F matrix{};
        matrix.SetRow(0, m_Data[0] / f);
        matrix.SetRow(1, m_Data[1] / f);
        matrix.SetRow(2, m_Data[2] / f);
        matrix.SetRow(3, m_Data[3] / f);
        return matrix;
    }

    Matrix4x4F& Matrix4x4F::operator*=(Float32 f)
    {
        *this = *this * f;
        return *this;
    }

    Matrix4x4F& Matrix4x4F::operator/=(Float32 f)
    {
        *this = *this / f;
        return *this;
    }

    Matrix4x4F Matrix4x4F::operator-() const
    {
        return *this * -1;
    }

    Vector4F Matrix4x4F::operator*(const Vector4F& vector) const
    {
        return Vector4F(Internal::SIMDMatrix4x4VectorMultiply(&m_Data[0].GetSIMD(), vector.GetSIMD()));
    }

    Vector3F Matrix4x4F::operator*(const Vector3F& vector) const
    {
        Vector4F v(vector, 1.0f);
        auto result = *this * v;
        return result.GetVector3F();
    }

    Matrix4x4F Matrix4x4F::Transposed() const
    {
        // TODO: SIMD?
        Matrix4x4F matrix{};
        matrix.SetRow(0, GetColumn(0));
        matrix.SetRow(1, GetColumn(1));
        matrix.SetRow(2, GetColumn(2));
        matrix.SetRow(3, GetColumn(3));
        return matrix;
    }

    void Matrix4x4F::Transpose()
    {
        *this = Transposed();
    }

    bool Matrix4x4F::IsApproxEqualTo(const Matrix4x4F& other, Float32 epsilon) const noexcept
    {
        for (size_t i = 0; i < RowCount; ++i)
        {
            if (!m_Data[i].IsApproxEqualTo(other.m_Data[i], epsilon))
            {
                return false;
            }
        }

        return true;
    }

    bool Matrix4x4F::operator==(const Matrix4x4F& other) const noexcept
    {
        for (size_t i = 0; i < RowCount; ++i)
        {
            if (m_Data[i] != other.m_Data[i])
            {
                return false;
            }
        }

        return true;
    }

    bool Matrix4x4F::operator!=(const Matrix4x4F& other) const noexcept
    {
        return !(*this == other);
    }

    Float32 Matrix4x4F::Determinant() const
    {
        static constexpr auto det3 = [](const Vector3F& a, const Vector3F& b, const Vector3F& c) {
            std::array<Vector3F, 3> m3{ a, b, c };
            return Internal::SIMDMatrix3x3Determinant(m3.data());
        };
        const auto& m = *this;

        auto d1 = det3({ m(1, 1), m(1, 2), m(1, 3) }, { m(2, 1), m(2, 2), m(2, 3) }, { m(3, 1), m(3, 2), m(3, 3) });
        auto d2 = det3({ m(1, 0), m(1, 2), m(1, 3) }, { m(2, 0), m(2, 2), m(2, 3) }, { m(3, 0), m(3, 2), m(3, 3) });
        auto d3 = det3({ m(1, 0), m(1, 1), m(1, 3) }, { m(2, 0), m(2, 1), m(2, 3) }, { m(3, 0), m(3, 1), m(3, 3) });
        auto d4 = det3({ m(1, 0), m(1, 1), m(1, 2) }, { m(2, 0), m(2, 1), m(2, 2) }, { m(3, 0), m(3, 1), m(3, 2) });

        return m(0, 0) * d1 - m(0, 1) * d2 + m(0, 2) * d3 - m(0, 3) * d4;
    }

    void Matrix4x4F::InvertTransform()
    {
        *this = InverseTransform();
    }

    Matrix4x4F Matrix4x4F::InverseTransform() const
    {
        using SIMD::SSE::Float32x4;

        const auto& t = *this;
        Matrix4x4F m{};
        auto s0 = t(0, 0) * t(1, 1) - t(1, 0) * t(0, 1);
        auto s1 = t(0, 0) * t(1, 2) - t(1, 0) * t(0, 2);
        auto s2 = t(0, 0) * t(1, 3) - t(1, 0) * t(0, 3);
        auto s3 = t(0, 1) * t(1, 2) - t(1, 1) * t(0, 2);
        auto s4 = t(0, 1) * t(1, 3) - t(1, 1) * t(0, 3);
        auto s5 = t(0, 2) * t(1, 3) - t(1, 2) * t(0, 3);

        auto c5 = t(2, 2) * t(3, 3) - t(3, 2) * t(2, 3);
        auto c4 = t(2, 1) * t(3, 3) - t(3, 1) * t(2, 3);
        auto c3 = t(2, 1) * t(3, 2) - t(3, 1) * t(2, 2);
        auto c2 = t(2, 0) * t(3, 3) - t(3, 0) * t(2, 3);
        auto c1 = t(2, 0) * t(3, 2) - t(3, 0) * t(2, 2);
        auto c0 = t(2, 0) * t(3, 1) - t(3, 0) * t(2, 1);

        auto invDet = 1 / (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);

        m(0, 0) = ( t(1, 1) * c5 - t(1, 2) * c4 + t(1, 3) * c3) * invDet;
        m(0, 1) = (-t(0, 1) * c5 + t(0, 2) * c4 - t(0, 3) * c3) * invDet;
        m(0, 2) = ( t(3, 1) * s5 - t(3, 2) * s4 + t(3, 3) * s3) * invDet;
        m(0, 3) = (-t(2, 1) * s5 + t(2, 2) * s4 - t(2, 3) * s3) * invDet;

        m(1, 0) = (-t(1, 0) * c5 + t(1, 2) * c2 - t(1, 3) * c1) * invDet;
        m(1, 1) = ( t(0, 0) * c5 - t(0, 2) * c2 + t(0, 3) * c1) * invDet;
        m(1, 2) = (-t(3, 0) * s5 + t(3, 2) * s2 - t(3, 3) * s1) * invDet;
        m(1, 3) = ( t(2, 0) * s5 - t(2, 2) * s2 + t(2, 3) * s1) * invDet;

        m(2, 0) = ( t(1, 0) * c4 - t(1, 1) * c2 + t(1, 3) * c0) * invDet;
        m(2, 1) = (-t(0, 0) * c4 + t(0, 1) * c2 - t(0, 3) * c0) * invDet;
        m(2, 2) = ( t(3, 0) * s4 - t(3, 1) * s2 + t(3, 3) * s0) * invDet;
        m(2, 3) = (-t(2, 0) * s4 + t(2, 1) * s2 - t(2, 3) * s0) * invDet;

        m(3, 0) = (-t(1, 0) * c3 + t(1, 1) * c1 - t(1, 2) * c0) * invDet;
        m(3, 1) = ( t(0, 0) * c3 - t(0, 1) * c1 + t(0, 2) * c0) * invDet;
        m(3, 2) = (-t(3, 0) * s3 + t(3, 1) * s1 - t(3, 2) * s0) * invDet;
        m(3, 3) = ( t(2, 0) * s3 - t(2, 1) * s1 + t(2, 2) * s0) * invDet;

        return m;
    }
} // namespace FE
