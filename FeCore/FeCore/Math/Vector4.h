#pragma once
#include <SIMD/CommonSIMD.h>
#include <cstdint>
#include <iostream>
#include <array>
#include "Vector3.h"

namespace FE
{
    class Vector4F
    {
        using TVec = SIMD::SSE::Float32x4;

        union
        {
            TVec m_Value;
            float m_Values[4];
            struct
            {
                float m_X, m_Y, m_Z, m_W;
            };
        };

        FE_FINLINE Vector4F(TVec vec) noexcept;

    public:
        Vector4F() = default;

        FE_FINLINE Vector4F(const Vector4F& other) noexcept;

        FE_FINLINE Vector4F(const Vector3F& other, float w = 1.0f) noexcept;

        FE_FINLINE Vector4F& operator=(const Vector4F& other) noexcept;

        FE_FINLINE Vector4F(Vector4F&& other) noexcept;

        FE_FINLINE Vector4F& operator=(Vector4F&& other) noexcept;

        FE_FINLINE explicit Vector4F(float value) noexcept;

        FE_FINLINE Vector4F(float x, float y, float z, float w) noexcept;

        FE_FINLINE Vector4F(const std::array<float, 4>& array) noexcept;

        FE_FINLINE static Vector4F GetZero() noexcept;

        FE_FINLINE static Vector4F GetUnitX() noexcept;

        FE_FINLINE static Vector4F GetUnitY() noexcept;

        FE_FINLINE static Vector4F GetUnitZ() noexcept;

        FE_FINLINE static Vector4F GetUnitW() noexcept;

        FE_FINLINE float operator[](size_t index) const noexcept;

        FE_FINLINE float operator()(size_t index) const noexcept;

        FE_FINLINE const float* Data() const noexcept;

        FE_FINLINE float X() const noexcept;

        FE_FINLINE float Y() const noexcept;

        FE_FINLINE float Z() const noexcept;

        FE_FINLINE float W() const noexcept;

        FE_FINLINE float& X() noexcept;

        FE_FINLINE float& Y() noexcept;

        FE_FINLINE float& Z() noexcept;

        FE_FINLINE float& W() noexcept;

        FE_FINLINE void Set(float x, float y, float z, float w) noexcept;

        FE_FINLINE float Dot(const Vector4F& other) const noexcept;

        FE_FINLINE float LengthSq() const noexcept;

        FE_FINLINE float Length() const noexcept;

        FE_FINLINE Vector4F Normalized() const noexcept;

        FE_FINLINE Vector4F Lerp(const Vector4F& dst, float f) const noexcept;

        FE_FINLINE Vector4F Cross(const Vector4F& other) const noexcept;

        FE_FINLINE Vector4F MulEach(const Vector4F& other) const noexcept;

        FE_FINLINE bool IsApproxEqualTo(const Vector4F& other, float epsilon = 0.0001f) const noexcept;

        FE_FINLINE bool operator==(const Vector4F& other) const noexcept;

        FE_FINLINE bool operator!=(const Vector4F& other) const noexcept;

        FE_FINLINE Vector4F operator-() const noexcept;

        FE_FINLINE Vector4F operator+(const Vector4F& other) const noexcept;

        FE_FINLINE Vector4F operator-(const Vector4F& other) const noexcept;

        FE_FINLINE Vector4F operator*(float f) const noexcept;

        FE_FINLINE Vector4F operator/(float f) const noexcept;

        FE_FINLINE Vector4F& operator+=(const Vector4F& other) noexcept;

        FE_FINLINE Vector4F& operator-=(const Vector4F& other) noexcept;

        FE_FINLINE Vector4F& operator*=(float f) noexcept;

        FE_FINLINE Vector4F& operator/=(float f) noexcept;
    };

    FE_FINLINE Vector4F::Vector4F(TVec vec) noexcept
        : m_Value(vec)
    {
    }

    FE_FINLINE Vector4F::Vector4F(const Vector4F& other) noexcept
        : m_Value(other.m_Value)
    {
    }

    FE_FINLINE Vector4F::Vector4F(const Vector3F& other, float w) noexcept
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

    FE_FINLINE Vector4F::Vector4F(float value) noexcept
        : m_Value(value)
    {
    }

    FE_FINLINE Vector4F::Vector4F(float x, float y, float z, float w) noexcept
        : m_Value(x, y, z, w)
    {
    }

    FE_FINLINE Vector4F::Vector4F(const std::array<float, 4>& array) noexcept
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

    FE_FINLINE float Vector4F::operator[](size_t index) const noexcept
    {
        return m_Values[index];
    }

    FE_FINLINE float Vector4F::operator()(size_t index) const noexcept
    {
        return m_Values[index];
    }

    FE_FINLINE const float* Vector4F::Data() const noexcept
    {
        return m_Values;
    }

    FE_FINLINE float Vector4F::X() const noexcept
    {
        return m_X;
    }

    FE_FINLINE float Vector4F::Y() const noexcept
    {
        return m_Y;
    }

    FE_FINLINE float Vector4F::Z() const noexcept
    {
        return m_Z;
    }

    FE_FINLINE float Vector4F::W() const noexcept
    {
        return m_W;
    }

    FE_FINLINE float& Vector4F::X() noexcept
    {
        return m_X;
    }

    FE_FINLINE float& Vector4F::Y() noexcept
    {
        return m_Y;
    }

    FE_FINLINE float& Vector4F::Z() noexcept
    {
        return m_Z;
    }

    FE_FINLINE float& Vector4F::W() noexcept
    {
        return m_W;
    }

    FE_FINLINE void Vector4F::Set(float x, float y, float z, float w) noexcept
    {
        m_Value = TVec(x, y, z, w);
    }

    FE_FINLINE float Vector4F::Dot(const Vector4F& other) const noexcept
    {
        TVec mul = m_Value * other.m_Value;
        TVec t = mul * mul.Shuffle<2, 3, 0, 1>();
        TVec r = t + t.Shuffle<1, 0, 2, 3>();
        return r.Select<0>();
    }

    FE_FINLINE float Vector4F::LengthSq() const noexcept
    {
        return Dot(*this);
    }

    FE_FINLINE float Vector4F::Length() const noexcept
    {
        return std::sqrt(LengthSq());
    }

    FE_FINLINE Vector4F Vector4F::Normalized() const noexcept
    {
        float len = Length();
        return m_Value / len;
    }

    FE_FINLINE Vector4F Vector4F::Lerp(const Vector4F& dst, float f) const noexcept
    {
        return (dst.m_Value - m_Value) * f + m_Value;
    }

    FE_FINLINE Vector4F Vector4F::Cross(const Vector4F& other) const noexcept
    {
        auto yzx1 = m_Value.Shuffle<3, 0, 2, 1>();
        auto zxy1 = m_Value.Shuffle<3, 1, 0, 2>();
        auto yzx2 = other.m_Value.Shuffle<3, 0, 2, 1>();
        auto zxy2 = other.m_Value.Shuffle<3, 1, 0, 2>();

        return yzx1 * zxy2 - zxy1 * yzx2;
    }

    FE_FINLINE Vector4F Vector4F::MulEach(const Vector4F& other) const noexcept
    {
        return m_Value * other.m_Value;
    }

    FE_FINLINE bool Vector4F::IsApproxEqualTo(const Vector4F& other, float epsilon) const noexcept
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

    FE_FINLINE Vector4F Vector4F::operator*(float f) const noexcept
    {
        return m_Value * f;
    }

    FE_FINLINE Vector4F Vector4F::operator/(float f) const noexcept
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

    FE_FINLINE Vector4F& Vector4F::operator*=(float f) noexcept
    {
        *this = *this * f;
        return *this;
    }

    FE_FINLINE Vector4F& Vector4F::operator/=(float f) noexcept
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
}
