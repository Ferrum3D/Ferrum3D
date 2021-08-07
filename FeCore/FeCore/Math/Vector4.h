#pragma once
#include <FeCore/Math/Vector3.h>
#include <FeCore/SIMD/CommonSIMD.h>
#include <array>
#include <cstdint>
#include <iostream>

namespace FE
{
    class Vector4F
    {
        using TVec = SIMD::SSE::Float32x4;

        union
        {
            TVec m_Value;
            Float32 m_Values[4];
            struct
            {
                Float32 m_X, m_Y, m_Z, m_W;
            };
        };

        FE_FINLINE Vector4F(TVec vec) noexcept;

    public:
        Vector4F() = default;

        FE_FINLINE Vector4F(const Vector4F& other) noexcept;

        FE_FINLINE Vector4F(const Vector3F& other, Float32 w = 1.0f) noexcept;

        FE_FINLINE Vector4F& operator=(const Vector4F& other) noexcept;

        FE_FINLINE Vector4F(Vector4F&& other) noexcept;

        FE_FINLINE Vector4F& operator=(Vector4F&& other) noexcept;

        FE_FINLINE explicit Vector4F(Float32 value) noexcept;

        FE_FINLINE Vector4F(Float32 x, Float32 y, Float32 z, Float32 w) noexcept;

        FE_FINLINE Vector4F(const std::array<Float32, 4>& array) noexcept;

        /**
         * @return Vector4F{ 0, 0, 0, 0 }.
        */
        FE_FINLINE static Vector4F GetZero() noexcept;

        /**
         * @return Vector4F{ 1, 0, 0, 0 }.
        */
        FE_FINLINE static Vector4F GetUnitX() noexcept;

        /**
         * @return Vector4F{ 0, 1, 0, 0 }.
        */
        FE_FINLINE static Vector4F GetUnitY() noexcept;

        /**
         * @return Vector4F{ 0, 0, 1, 0 }.
        */
        FE_FINLINE static Vector4F GetUnitZ() noexcept;

        /**
         * @return Vector4F{ 0, 0, 0, 1 }.
        */
        FE_FINLINE static Vector4F GetUnitW() noexcept;

        FE_FINLINE Float32 operator[](size_t index) const noexcept;

        FE_FINLINE Float32 operator()(size_t index) const noexcept;

        /**
         * @return pointer to array of four floats.
        */
        FE_FINLINE const Float32* Data() const noexcept;

        /**
         * @return Underlying SIMD type.
        */
        FE_FINLINE TVec GetSIMDVector() const noexcept;

        FE_FINLINE Float32 X() const noexcept;
        FE_FINLINE Float32 Y() const noexcept;
        FE_FINLINE Float32 Z() const noexcept;
        FE_FINLINE Float32 W() const noexcept;

        FE_FINLINE Float32& X() noexcept;
        FE_FINLINE Float32& Y() noexcept;
        FE_FINLINE Float32& Z() noexcept;
        FE_FINLINE Float32& W() noexcept;

        FE_FINLINE void Set(Float32 x, Float32 y, Float32 z, Float32 w) noexcept;

        FE_FINLINE Float32 Dot(const Vector4F& other) const noexcept;

        /**
         * @return Squared length of the vector.
        */
        FE_FINLINE Float32 LengthSq() const noexcept;

        /**
         * @return Length of the vector.
        */
        FE_FINLINE Float32 Length() const noexcept;

        /**
         * @return New normalized vector, this vector is not modified.
        */
        FE_FINLINE Vector4F Normalized() const noexcept;

        /**
         * @brief Linearly interpolate between this and destination.
         * 
         * The result is (dst - this) * f + this;
         * 
         * @param f Interpolation factor.
         * @return New interpolated vector, this vector is not modified.
        */
        FE_FINLINE Vector4F Lerp(const Vector4F& dst, Float32 f) const noexcept;

        /**
         * @brief Multiply each component of this vector with each component of other vector.
         * @return New vector, this vector is not modified.
        */
        FE_FINLINE Vector4F MulEach(const Vector4F& other) const noexcept;

        FE_FINLINE bool IsApproxEqualTo(const Vector4F& other, Float32 epsilon = 0.0001f) const noexcept;

        FE_FINLINE bool operator==(const Vector4F& other) const noexcept;

        FE_FINLINE bool operator!=(const Vector4F& other) const noexcept;

        FE_FINLINE Vector4F operator-() const noexcept;

        FE_FINLINE Vector4F operator+(const Vector4F& other) const noexcept;

        FE_FINLINE Vector4F operator-(const Vector4F& other) const noexcept;

        FE_FINLINE Vector4F operator*(Float32 f) const noexcept;

        FE_FINLINE Vector4F operator/(Float32 f) const noexcept;

        FE_FINLINE Vector4F& operator+=(const Vector4F& other) noexcept;

        FE_FINLINE Vector4F& operator-=(const Vector4F& other) noexcept;

        FE_FINLINE Vector4F& operator*=(Float32 f) noexcept;

        FE_FINLINE Vector4F& operator/=(Float32 f) noexcept;
    };

    FE_FINLINE Vector4F::Vector4F(TVec vec) noexcept
        : m_Value(vec)
    {
    }

    FE_FINLINE Vector4F::Vector4F(const Vector4F& other) noexcept
        : m_Value(other.m_Value)
    {
    }

    FE_FINLINE Vector4F::Vector4F(const Vector3F& other, Float32 w) noexcept
        : m_Value(other.GetSIMDVector())
    {
        m_W = w;
    }

    FE_FINLINE Vector4F& Vector4F::operator=(const Vector4F& other) noexcept
    {
        m_Value = other.m_Value;
        return *this;
    }

    FE_FINLINE Vector4F::Vector4F(Vector4F&& other) noexcept
        : m_Value(other.m_Value)
    {
    }

    FE_FINLINE Vector4F& Vector4F::operator=(Vector4F&& other) noexcept
    {
        m_Value = other.m_Value;
        return *this;
    }

    FE_FINLINE Vector4F::Vector4F(Float32 value) noexcept
        : m_Value(value)
    {
    }

    FE_FINLINE Vector4F::Vector4F(Float32 x, Float32 y, Float32 z, Float32 w) noexcept
        : m_Value(x, y, z, w)
    {
    }

    FE_FINLINE Vector4F::Vector4F(const std::array<Float32, 4>& array) noexcept
        : m_Value(array[0], array[1], array[2], array[3])
    {
    }

    FE_FINLINE Vector4F Vector4F::GetZero() noexcept
    {
        return Vector4F(0);
    }

    FE_FINLINE Vector4F Vector4F::GetUnitX() noexcept
    {
        return Vector4F(1, 0, 0, 0);
    }

    FE_FINLINE Vector4F Vector4F::GetUnitY() noexcept
    {
        return Vector4F(0, 1, 0, 0);
    }

    FE_FINLINE Vector4F Vector4F::GetUnitZ() noexcept
    {
        return Vector4F(0, 0, 1, 0);
    }

    FE_FINLINE Vector4F Vector4F::GetUnitW() noexcept
    {
        return Vector4F(0, 0, 0, 1);
    }

    FE_FINLINE Float32 Vector4F::operator[](size_t index) const noexcept
    {
        return m_Values[index];
    }

    FE_FINLINE Float32 Vector4F::operator()(size_t index) const noexcept
    {
        return m_Values[index];
    }

    FE_FINLINE const Float32* Vector4F::Data() const noexcept
    {
        return m_Values;
    }

    FE_FINLINE Vector4F::TVec Vector4F::GetSIMDVector() const noexcept
    {
        return m_Value;
    }

    FE_FINLINE Float32 Vector4F::X() const noexcept
    {
        return m_X;
    }

    FE_FINLINE Float32 Vector4F::Y() const noexcept
    {
        return m_Y;
    }

    FE_FINLINE Float32 Vector4F::Z() const noexcept
    {
        return m_Z;
    }

    FE_FINLINE Float32 Vector4F::W() const noexcept
    {
        return m_W;
    }

    FE_FINLINE Float32& Vector4F::X() noexcept
    {
        return m_X;
    }

    FE_FINLINE Float32& Vector4F::Y() noexcept
    {
        return m_Y;
    }

    FE_FINLINE Float32& Vector4F::Z() noexcept
    {
        return m_Z;
    }

    FE_FINLINE Float32& Vector4F::W() noexcept
    {
        return m_W;
    }

    FE_FINLINE void Vector4F::Set(Float32 x, Float32 y, Float32 z, Float32 w) noexcept
    {
        m_Value = TVec(x, y, z, w);
    }

    FE_FINLINE Float32 Vector4F::Dot(const Vector4F& other) const noexcept
    {
        TVec mul = m_Value * other.m_Value;
        TVec t   = mul * mul.Shuffle<2, 3, 0, 1>();
        TVec r   = t + t.Shuffle<1, 0, 2, 3>();
        return r.Select<0>();
    }

    FE_FINLINE Float32 Vector4F::LengthSq() const noexcept
    {
        return Dot(*this);
    }

    FE_FINLINE Float32 Vector4F::Length() const noexcept
    {
        return std::sqrt(LengthSq());
    }

    FE_FINLINE Vector4F Vector4F::Normalized() const noexcept
    {
        Float32 len = Length();
        return m_Value / len;
    }

    FE_FINLINE Vector4F Vector4F::Lerp(const Vector4F& dst, Float32 f) const noexcept
    {
        return (dst.m_Value - m_Value) * f + m_Value;
    }

    FE_FINLINE Vector4F Vector4F::MulEach(const Vector4F& other) const noexcept
    {
        return m_Value * other.m_Value;
    }

    FE_FINLINE bool Vector4F::IsApproxEqualTo(const Vector4F& other, Float32 epsilon) const noexcept
    {
        // TODO: move epsilon value to some <MathUtils.h>
        return TVec::CompareAllLe((m_Value - other.m_Value).Abs(), epsilon, 0xfff);
    }

    FE_FINLINE bool Vector4F::operator==(const Vector4F& other) const noexcept
    {
        return TVec::CompareAllEq(m_Value, other.m_Value, 0xfff);
    }

    FE_FINLINE bool Vector4F::operator!=(const Vector4F& other) const noexcept
    {
        return TVec::CompareAllNeq(m_Value, other.m_Value, 0xfff);
    }

    FE_FINLINE Vector4F Vector4F::operator-() const noexcept
    {
        return 1.0f - m_Value;
    }

    FE_FINLINE Vector4F Vector4F::operator+(const Vector4F& other) const noexcept
    {
        return m_Value + other.m_Value;
    }

    FE_FINLINE Vector4F Vector4F::operator-(const Vector4F& other) const noexcept
    {
        return m_Value - other.m_Value;
    }

    FE_FINLINE Vector4F Vector4F::operator*(Float32 f) const noexcept
    {
        return m_Value * f;
    }

    FE_FINLINE Vector4F Vector4F::operator/(Float32 f) const noexcept
    {
        return m_Value / f;
    }

    FE_FINLINE Vector4F& Vector4F::operator+=(const Vector4F& other) noexcept
    {
        *this = *this + other;
        return *this;
    }

    FE_FINLINE Vector4F& Vector4F::operator-=(const Vector4F& other) noexcept
    {
        *this = *this - other;
        return *this;
    }

    FE_FINLINE Vector4F& Vector4F::operator*=(Float32 f) noexcept
    {
        *this = *this * f;
        return *this;
    }

    FE_FINLINE Vector4F& Vector4F::operator/=(Float32 f) noexcept
    {
        *this = *this / f;
        return *this;
    }

    using float4 = Vector4F;
} // namespace FE

namespace std
{
    inline ostream& operator<<(ostream& stream, const FE::float4& vec)
    {
        return stream << "{ " << vec.X() << "; " << vec.Y() << "; " << vec.Z() << "; " << vec.W() << " }";
    }
} // namespace std
