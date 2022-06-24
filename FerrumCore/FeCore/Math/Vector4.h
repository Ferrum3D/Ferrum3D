#pragma once
#include <FeCore/Math/Vector3.h>
#include <FeCore/SIMD/CommonSIMD.h>
#include <array>
#include <cstdint>
#include <iostream>

namespace FE
{
    //! \brief 4-dimensional vector.
    class Vector4F
    {
        using TVec = SIMD::SSE::Float32x4;

        FE_PUSH_MSVC_WARNING(4201)
        union
        {
            TVec m_Value;
            Float32 m_Values[4];
            struct
            {
                Float32 m_X, m_Y, m_Z, m_W;
            };
        };
        FE_POP_MSVC_WARNING

    public:
        FE_STRUCT_RTTI(Vector4F, "C8B28F38-DAAB-4C9A-A922-41E881ED184C");

        FE_FINLINE Vector4F()
            : Vector4F(0, 0, 0, 0)
        {
        }

        FE_FINLINE explicit Vector4F(TVec vec) noexcept;

        FE_FINLINE Vector4F(const Vector4F& other) noexcept;

        FE_FINLINE explicit Vector4F(const Vector3F& other, Float32 w = 1.0f) noexcept;

        FE_FINLINE Vector4F& operator=(const Vector4F& other) noexcept;

        FE_FINLINE Vector4F(Vector4F&& other) noexcept;

        FE_FINLINE Vector4F& operator=(Vector4F&& other) noexcept;

        FE_FINLINE explicit Vector4F(Float32 value) noexcept;

        FE_FINLINE Vector4F(Float32 x, Float32 y, Float32 z, Float32 w) noexcept;

        [[nodiscard]] FE_FINLINE Vector3F GetVector3F() const noexcept;

        FE_FINLINE explicit Vector4F(const std::array<Float32, 4>& array) noexcept;

        //! \return Vector4F{ 0, 0, 0, 0 }.
        [[nodiscard]] FE_FINLINE static Vector4F GetZero() noexcept;

        //! \return Vector4F{ 1, 0, 0, 0 }.
        [[nodiscard]] FE_FINLINE static Vector4F GetUnitX() noexcept;

        //! \return Vector4F{ 0, 1, 0, 0 }.
        [[nodiscard]] FE_FINLINE static Vector4F GetUnitY() noexcept;

        //! \return Vector4F{ 0, 0, 1, 0 }.
        [[nodiscard]] FE_FINLINE static Vector4F GetUnitZ() noexcept;

        //! \return Vector4F{ 0, 0, 0, 1 }.
        [[nodiscard]] FE_FINLINE static Vector4F GetUnitW() noexcept;

        [[nodiscard]] FE_FINLINE Float32 operator[](size_t index) const noexcept;

        [[nodiscard]] FE_FINLINE Float32& operator()(size_t index) noexcept;
        [[nodiscard]] FE_FINLINE Float32 operator()(size_t index) const noexcept;

        //! \return A pointer to array of four floats (components of the vector).
        [[nodiscard]] FE_FINLINE const Float32* Data() const noexcept;

        //! \return Underlying SIMD type.
        [[nodiscard]] FE_FINLINE const TVec& GetSIMD() const noexcept;

        [[nodiscard]] FE_FINLINE Float32 X() const noexcept;
        [[nodiscard]] FE_FINLINE Float32 Y() const noexcept;
        [[nodiscard]] FE_FINLINE Float32 Z() const noexcept;
        [[nodiscard]] FE_FINLINE Float32 W() const noexcept;

        [[nodiscard]] FE_FINLINE Float32& X() noexcept;
        [[nodiscard]] FE_FINLINE Float32& Y() noexcept;
        [[nodiscard]] FE_FINLINE Float32& Z() noexcept;
        [[nodiscard]] FE_FINLINE Float32& W() noexcept;

        FE_FINLINE void Set(Float32 x, Float32 y, Float32 z, Float32 w) noexcept;

        [[nodiscard]] FE_FINLINE Float32 Dot(const Vector4F& other) const noexcept;

        //! \return Squared length of the vector.
        [[nodiscard]] FE_FINLINE Float32 LengthSq() const noexcept;

        //! \return Length of the vector.
        [[nodiscard]] FE_FINLINE Float32 Length() const noexcept;

        //! \return New normalized vector, this vector is not modified.
        [[nodiscard]] FE_FINLINE Vector4F Normalized() const noexcept;

        //! \brief Linearly interpolate between this and destination.
        //!
        //! The result is (dst - this) * f + this;
        //!
        //! \param [in] f - Interpolation factor.
        //!
        //! \return New interpolated vector, this vector is not modified.
        [[nodiscard]] FE_FINLINE Vector4F Lerp(const Vector4F& dst, Float32 f) const noexcept;

        //! \brief Multiply each component of this vector with each component of other vector.
        //! \return New vector, this vector is not modified.
        [[nodiscard]] FE_FINLINE Vector4F MulEach(const Vector4F& other) const noexcept;

        //! \brief Check if two vectors are approximately equal.
        //!
        //! \param [in] other   - The vector to compare this vector with.
        //! \param [in] epsilon - Accepted difference between the two vectors.
        //!
        //! \return True if the vectors are approximately equal.
        [[nodiscard]] FE_FINLINE bool IsApproxEqualTo(const Vector4F& other, Float32 epsilon = Constants::Epsilon) const noexcept;

        [[nodiscard]] FE_FINLINE bool operator==(const Vector4F& other) const noexcept;
        [[nodiscard]] FE_FINLINE bool operator!=(const Vector4F& other) const noexcept;

        [[nodiscard]] FE_FINLINE Vector4F operator-() const noexcept;

        [[nodiscard]] FE_FINLINE Vector4F operator+(const Vector4F& other) const noexcept;
        [[nodiscard]] FE_FINLINE Vector4F operator-(const Vector4F& other) const noexcept;
        [[nodiscard]] FE_FINLINE Vector4F operator*(Float32 f) const noexcept;
        [[nodiscard]] FE_FINLINE Vector4F operator/(Float32 f) const noexcept;

        FE_FINLINE Vector4F& operator+=(const Vector4F& other) noexcept;
        FE_FINLINE Vector4F& operator-=(const Vector4F& other) noexcept;
        FE_FINLINE Vector4F& operator*=(Float32 f) noexcept;
        FE_FINLINE Vector4F& operator/=(Float32 f) noexcept;
    };

    Vector4F::Vector4F(TVec vec) noexcept // NOLINT clang-tidy complains about uninitialized union members
        : m_Value(vec)
    {
    }

    Vector4F::Vector4F(const Vector4F& other) noexcept // NOLINT
        : m_Value(other.m_Value)
    {
    }

    Vector4F::Vector4F(const Vector3F& other, Float32 w) noexcept // NOLINT
        : m_Value(other.GetSIMD())
    {
        m_W = w;
    }

    Vector4F& Vector4F::operator=(const Vector4F& other) noexcept
    {
        m_Value = other.m_Value;
        return *this;
    }

    Vector4F::Vector4F(Vector4F&& other) noexcept // NOLINT
        : m_Value(other.m_Value)
    {
    }

    Vector4F& Vector4F::operator=(Vector4F&& other) noexcept
    {
        m_Value = other.m_Value;
        return *this;
    }

    Vector4F::Vector4F(Float32 value) noexcept // NOLINT
        : m_Value(value)
    {
    }

    Vector4F::Vector4F(Float32 x, Float32 y, Float32 z, Float32 w) noexcept // NOLINT
        : m_Value(x, y, z, w)
    {
    }

    Vector4F::Vector4F(const std::array<Float32, 4>& array) noexcept // NOLINT
        : m_Value(array[0], array[1], array[2], array[3])
    {
    }

    Vector3F Vector4F::GetVector3F() const noexcept
    {
        return Vector3F(m_Value);
    }

    Vector4F Vector4F::GetZero() noexcept
    {
        return Vector4F(0);
    }

    Vector4F Vector4F::GetUnitX() noexcept
    {
        return Vector4F(1, 0, 0, 0);
    }

    Vector4F Vector4F::GetUnitY() noexcept
    {
        return Vector4F(0, 1, 0, 0);
    }

    Vector4F Vector4F::GetUnitZ() noexcept
    {
        return Vector4F(0, 0, 1, 0);
    }

    Vector4F Vector4F::GetUnitW() noexcept
    {
        return Vector4F(0, 0, 0, 1);
    }

    Float32 Vector4F::operator[](size_t index) const noexcept
    {
        return m_Values[index];
    }

    Float32 Vector4F::operator()(size_t index) const noexcept
    {
        return m_Values[index];
    }

    Float32& Vector4F::operator()(size_t index) noexcept
    {
        return m_Values[index];
    }

    const Float32* Vector4F::Data() const noexcept
    {
        return m_Values;
    }

    const Vector4F::TVec& Vector4F::GetSIMD() const noexcept
    {
        return m_Value;
    }

    Float32 Vector4F::X() const noexcept
    {
        return m_X;
    }

    Float32 Vector4F::Y() const noexcept
    {
        return m_Y;
    }

    Float32 Vector4F::Z() const noexcept
    {
        return m_Z;
    }

    Float32 Vector4F::W() const noexcept
    {
        return m_W;
    }

    Float32& Vector4F::X() noexcept
    {
        return m_X;
    }

    Float32& Vector4F::Y() noexcept
    {
        return m_Y;
    }

    Float32& Vector4F::Z() noexcept
    {
        return m_Z;
    }

    Float32& Vector4F::W() noexcept
    {
        return m_W;
    }

    void Vector4F::Set(Float32 x, Float32 y, Float32 z, Float32 w) noexcept
    {
        m_Value = TVec(x, y, z, w);
    }

    Float32 Vector4F::Dot(const Vector4F& other) const noexcept
    {
        TVec mul = m_Value * other.m_Value;
        TVec t   = mul * mul.Shuffle<2, 3, 0, 1>();
        TVec r   = t + t.Shuffle<1, 0, 2, 3>();
        return r.Select<0>();
    }

    Float32 Vector4F::LengthSq() const noexcept
    {
        return Dot(*this);
    }

    Float32 Vector4F::Length() const noexcept
    {
        return std::sqrt(LengthSq());
    }

    Vector4F Vector4F::Normalized() const noexcept
    {
        Float32 len = Length();
        return Vector4F(m_Value / len);
    }

    Vector4F Vector4F::Lerp(const Vector4F& dst, Float32 f) const noexcept
    {
        return Vector4F((dst.m_Value - m_Value) * f + m_Value);
    }

    Vector4F Vector4F::MulEach(const Vector4F& other) const noexcept
    {
        return Vector4F(m_Value * other.m_Value);
    }

    bool Vector4F::IsApproxEqualTo(const Vector4F& other, Float32 epsilon) const noexcept
    {
        // TODO: move epsilon value to some <MathUtils.h>
        return TVec::CompareAllLe((m_Value - other.m_Value).Abs(), epsilon, 0xffff);
    }

    bool Vector4F::operator==(const Vector4F& other) const noexcept
    {
        return TVec::CompareAllEq(m_Value, other.m_Value, 0xffff);
    }

    bool Vector4F::operator!=(const Vector4F& other) const noexcept
    {
        return TVec::CompareAllNeq(m_Value, other.m_Value, 0xffff);
    }

    Vector4F Vector4F::operator-() const noexcept
    {
        return Vector4F(1.0f - m_Value);
    }

    Vector4F Vector4F::operator+(const Vector4F& other) const noexcept
    {
        return Vector4F(m_Value + other.m_Value);
    }

    Vector4F Vector4F::operator-(const Vector4F& other) const noexcept
    {
        return Vector4F(m_Value - other.m_Value);
    }

    Vector4F Vector4F::operator*(Float32 f) const noexcept
    {
        return Vector4F(m_Value * f);
    }

    Vector4F Vector4F::operator/(Float32 f) const noexcept
    {
        return Vector4F(m_Value / f);
    }

    Vector4F& Vector4F::operator+=(const Vector4F& other) noexcept
    {
        *this = *this + other;
        return *this;
    }

    Vector4F& Vector4F::operator-=(const Vector4F& other) noexcept
    {
        *this = *this - other;
        return *this;
    }

    Vector4F& Vector4F::operator*=(Float32 f) noexcept
    {
        *this = *this * f;
        return *this;
    }

    Vector4F& Vector4F::operator/=(Float32 f) noexcept
    {
        *this = *this / f;
        return *this;
    }
} // namespace FE

namespace std // NOLINT
{
    inline ostream& operator<<(ostream& stream, const FE::Vector4F& vec)
    {
        return stream << "{ " << vec.X() << "; " << vec.Y() << "; " << vec.Z() << "; " << vec.W() << " }";
    }
} // namespace std
