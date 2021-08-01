#pragma once
#include <FeCore/SIMD/CommonSIMD.h>
#include <array>
#include <cstdint>
#include <iostream>

namespace FE
{
    class Vector3F
    {
        using TVec = SIMD::SSE::Float32x4;

        union
        {
            TVec m_Value;
            float m_Values[3];
            struct
            {
                float m_X, m_Y, m_Z;
            };
        };

        FE_FINLINE Vector3F(TVec vec) noexcept;

    public:
        Vector3F() = default;

        FE_FINLINE Vector3F(const Vector3F& other) noexcept;

        FE_FINLINE Vector3F& operator=(const Vector3F& other) noexcept;

        FE_FINLINE Vector3F(Vector3F&& other) noexcept;

        FE_FINLINE Vector3F& operator=(Vector3F&& other) noexcept;

        FE_FINLINE explicit Vector3F(float value) noexcept;

        FE_FINLINE Vector3F(float x, float y, float z) noexcept;

        FE_FINLINE Vector3F(const std::array<float, 3>& array) noexcept;

        FE_FINLINE static Vector3F GetZero() noexcept;

        FE_FINLINE static Vector3F GetUnitX() noexcept;

        FE_FINLINE static Vector3F GetUnitY() noexcept;

        FE_FINLINE static Vector3F GetUnitZ() noexcept;

        FE_FINLINE float operator[](size_t index) const noexcept;

        FE_FINLINE float operator()(size_t index) const noexcept;

        FE_FINLINE const float* Data() const noexcept;

        FE_FINLINE TVec GetSIMDVector() const noexcept;

        FE_FINLINE float X() const noexcept;

        FE_FINLINE float Y() const noexcept;

        FE_FINLINE float Z() const noexcept;

        FE_FINLINE float& X() noexcept;

        FE_FINLINE float& Y() noexcept;

        FE_FINLINE float& Z() noexcept;

        FE_FINLINE void Set(float x, float y, float z) noexcept;

        FE_FINLINE float Dot(const Vector3F& other) const noexcept;

        FE_FINLINE float LengthSq() const noexcept;

        FE_FINLINE float Length() const noexcept;

        FE_FINLINE Vector3F Normalized() const noexcept;

        FE_FINLINE Vector3F Lerp(const Vector3F& dst, float f) const noexcept;

        FE_FINLINE Vector3F Cross(const Vector3F& other) const noexcept;

        FE_FINLINE Vector3F MulEach(const Vector3F& other) const noexcept;

        FE_FINLINE bool IsApproxEqualTo(const Vector3F& other, float epsilon = 0.0001f) const noexcept;

        FE_FINLINE bool operator==(const Vector3F& other) const noexcept;

        FE_FINLINE bool operator!=(const Vector3F& other) const noexcept;

        FE_FINLINE Vector3F operator-() const noexcept;

        FE_FINLINE Vector3F operator+(const Vector3F& other) const noexcept;

        FE_FINLINE Vector3F operator-(const Vector3F& other) const noexcept;

        FE_FINLINE Vector3F operator*(float f) const noexcept;

        FE_FINLINE Vector3F operator/(float f) const noexcept;

        FE_FINLINE Vector3F& operator+=(const Vector3F& other) noexcept;

        FE_FINLINE Vector3F& operator-=(const Vector3F& other) noexcept;

        FE_FINLINE Vector3F& operator*=(float f) noexcept;

        FE_FINLINE Vector3F& operator/=(float f) noexcept;
    };

    FE_FINLINE Vector3F::Vector3F(TVec vec) noexcept
        : m_Value(vec)
    {
    }

    FE_FINLINE Vector3F::Vector3F(const Vector3F& other) noexcept
        : m_Value(other.m_Value)
    {
    }

    FE_FINLINE Vector3F& Vector3F::operator=(const Vector3F& other) noexcept
    {
        m_Value = other.m_Value;
        return *this;
    }

    FE_FINLINE Vector3F::Vector3F(Vector3F&& other) noexcept
        : m_Value(other.m_Value)
    {
    }

    FE_FINLINE Vector3F& Vector3F::operator=(Vector3F&& other) noexcept
    {
        m_Value = other.m_Value;
        return *this;
    }

    FE_FINLINE Vector3F::Vector3F(float value) noexcept
        : m_Value(value)
    {
    }

    FE_FINLINE Vector3F::Vector3F(float x, float y, float z) noexcept
        : m_Value(x, y, z)
    {
    }

    FE_FINLINE Vector3F::Vector3F(const std::array<float, 3>& array) noexcept
        : m_Value(array[0], array[1], array[2])
    {
    }

    FE_FINLINE Vector3F Vector3F::GetZero() noexcept
    {
        return Vector3F(0);
    }

    FE_FINLINE Vector3F Vector3F::GetUnitX() noexcept
    {
        return Vector3F(1, 0, 0);
    }

    FE_FINLINE Vector3F Vector3F::GetUnitY() noexcept
    {
        return Vector3F(0, 1, 0);
    }

    FE_FINLINE Vector3F Vector3F::GetUnitZ() noexcept
    {
        return Vector3F(0, 0, 1);
    }

    FE_FINLINE float Vector3F::operator[](size_t index) const noexcept
    {
        return m_Values[index];
    }

    FE_FINLINE float Vector3F::operator()(size_t index) const noexcept
    {
        return m_Values[index];
    }

    FE_FINLINE const float* Vector3F::Data() const noexcept
    {
        return m_Values;
    }

    FE_FINLINE Vector3F::TVec Vector3F::GetSIMDVector() const noexcept
    {
        return m_Value;
    }

    FE_FINLINE float Vector3F::X() const noexcept
    {
        return m_X;
    }

    FE_FINLINE float Vector3F::Y() const noexcept
    {
        return m_Y;
    }

    FE_FINLINE float Vector3F::Z() const noexcept
    {
        return m_Z;
    }

    FE_FINLINE float& Vector3F::X() noexcept
    {
        return m_X;
    }

    FE_FINLINE float& Vector3F::Y() noexcept
    {
        return m_Y;
    }

    FE_FINLINE float& Vector3F::Z() noexcept
    {
        return m_Z;
    }

    FE_FINLINE void Vector3F::Set(float x, float y, float z) noexcept
    {
        m_Value = TVec(x, y, z);
    }

    FE_FINLINE float Vector3F::Dot(const Vector3F& other) const noexcept
    {
        TVec mul     = m_Value * other.m_Value;
        TVec halfSum = mul.Broadcast<1>() + mul;
        TVec sum     = mul.Broadcast<2>() + halfSum;
        return sum.Select<0>();
    }

    FE_FINLINE float Vector3F::LengthSq() const noexcept
    {
        return Dot(*this);
    }

    FE_FINLINE float Vector3F::Length() const noexcept
    {
        return std::sqrt(LengthSq());
    }

    FE_FINLINE Vector3F Vector3F::Normalized() const noexcept
    {
        float len = Length();
        return m_Value / len;
    }

    FE_FINLINE Vector3F Vector3F::Lerp(const Vector3F& dst, float f) const noexcept
    {
        return (dst.m_Value - m_Value) * f + m_Value;
    }

    FE_FINLINE Vector3F Vector3F::Cross(const Vector3F& other) const noexcept
    {
        auto yzx1 = m_Value.Shuffle<3, 0, 2, 1>();
        auto zxy1 = m_Value.Shuffle<3, 1, 0, 2>();
        auto yzx2 = other.m_Value.Shuffle<3, 0, 2, 1>();
        auto zxy2 = other.m_Value.Shuffle<3, 1, 0, 2>();

        return yzx1 * zxy2 - zxy1 * yzx2;
    }

    FE_FINLINE Vector3F Vector3F::MulEach(const Vector3F& other) const noexcept
    {
        return m_Value * other.m_Value;
    }

    FE_FINLINE bool Vector3F::IsApproxEqualTo(const Vector3F& other, float epsilon) const noexcept
    {
        // TODO: move epsilon value to some <MathUtils.h>
        return TVec::CompareAllLe((m_Value - other.m_Value).Abs(), epsilon, 0xfff);
    }

    FE_FINLINE bool Vector3F::operator==(const Vector3F& other) const noexcept
    {
        return TVec::CompareAllEq(m_Value, other.m_Value, 0xfff);
    }

    FE_FINLINE bool Vector3F::operator!=(const Vector3F& other) const noexcept
    {
        return TVec::CompareAllNeq(m_Value, other.m_Value, 0xfff);
    }

    FE_FINLINE Vector3F Vector3F::operator-() const noexcept
    {
        return 1.0f - m_Value;
    }

    FE_FINLINE Vector3F Vector3F::operator+(const Vector3F& other) const noexcept
    {
        return m_Value + other.m_Value;
    }

    FE_FINLINE Vector3F Vector3F::operator-(const Vector3F& other) const noexcept
    {
        return m_Value - other.m_Value;
    }

    FE_FINLINE Vector3F Vector3F::operator*(float f) const noexcept
    {
        return m_Value * f;
    }

    FE_FINLINE Vector3F Vector3F::operator/(float f) const noexcept
    {
        return m_Value / f;
    }

    FE_FINLINE Vector3F& Vector3F::operator+=(const Vector3F& other) noexcept
    {
        *this = *this + other;
        return *this;
    }

    FE_FINLINE Vector3F& Vector3F::operator-=(const Vector3F& other) noexcept
    {
        *this = *this - other;
        return *this;
    }

    FE_FINLINE Vector3F& Vector3F::operator*=(float f) noexcept
    {
        *this = *this * f;
        return *this;
    }

    FE_FINLINE Vector3F& Vector3F::operator/=(float f) noexcept
    {
        *this = *this / f;
        return *this;
    }

    using float3 = Vector3F;
} // namespace FE

namespace std
{
    inline ostream& operator<<(ostream& stream, const FE::float3& vec)
    {
        return stream << "{ " << vec.X() << "; " << vec.Y() << "; " << vec.Z() << " }";
    }
} // namespace std
