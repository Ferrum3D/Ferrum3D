#pragma once
#include <FeCore/RTTI/RTTI.h>
#include <FeCore/SIMD/CommonSIMD.h>
#include <array>
#include <cstdint>
#include <iostream>

namespace FE
{
    //! \brief 3-dimentional vector.
    class Vector3F
    {
        using TVec = SIMD::SSE::Float32x4;

        FE_PUSH_MSVC_WARNING(4201)
        union
        {
            TVec m_Value;
            Float32 m_Values[3];
            struct
            {
                Float32 m_X, m_Y, m_Z;
            };
        };
        FE_POP_MSVC_WARNING

        FE_FINLINE Vector3F(TVec vec) noexcept;

    public:
        FE_STRUCT_RTTI(Vector3F, "FBD32DD3-C4C4-46DA-8F74-E9EA863BCAAD");

        FE_FINLINE Vector3F()
            : Vector3F(0, 0, 0)
        {
        }

        FE_FINLINE Vector3F(const Vector3F& other) noexcept;

        FE_FINLINE Vector3F& operator=(const Vector3F& other) noexcept;

        FE_FINLINE Vector3F(Vector3F&& other) noexcept;

        FE_FINLINE Vector3F& operator=(Vector3F&& other) noexcept;

        FE_FINLINE explicit Vector3F(Float32 value) noexcept;

        FE_FINLINE Vector3F(Float32 x, Float32 y, Float32 z) noexcept;

        FE_FINLINE Vector3F(const std::array<Float32, 3>& array) noexcept;

        //! \return Vector3F{ 0, 0, 0 }.
        FE_FINLINE static Vector3F GetZero() noexcept;

        //! \return Vector3F{ 1, 0, 0 }.
        FE_FINLINE static Vector3F GetUnitX() noexcept;

        //! \return Vector3F{ 0, 1, 0 }.
        FE_FINLINE static Vector3F GetUnitY() noexcept;

        //! \return Vector3F{ 0, 0, 1 }.
        FE_FINLINE static Vector3F GetUnitZ() noexcept;

        FE_FINLINE Float32 operator[](size_t index) const noexcept;

        FE_FINLINE Float32 operator()(size_t index) const noexcept;

        //! \return A pointer to array of three floats (components of the vector).
        FE_FINLINE const Float32* Data() const noexcept;

        //! \return Underlying SIMD type.
        FE_FINLINE TVec GetSIMDVector() const noexcept;

        FE_FINLINE Float32 X() const noexcept;
        FE_FINLINE Float32 Y() const noexcept;
        FE_FINLINE Float32 Z() const noexcept;

        FE_FINLINE Float32& X() noexcept;
        FE_FINLINE Float32& Y() noexcept;
        FE_FINLINE Float32& Z() noexcept;

        FE_FINLINE void Set(Float32 x, Float32 y, Float32 z) noexcept;

        FE_FINLINE Float32 Dot(const Vector3F& other) const noexcept;

        //! \return Squared length of the vector.
        FE_FINLINE Float32 LengthSq() const noexcept;

        //! \return Length of the vector.
        FE_FINLINE Float32 Length() const noexcept;

        //! \return New normalized vector, this vector is not modified.
        FE_FINLINE Vector3F Normalized() const noexcept;

        //! \brief Linearly interpolate between this and destination.
        //!
        //! The result is (dst - this) * f + this.
        //!
        //! \param [in] f - Interpolation factor.
        //!
        //! \return New interpolated vector, this vector is not modified.
        FE_FINLINE Vector3F Lerp(const Vector3F& dst, Float32 f) const noexcept;

        //! \return Cross product [this x other].
        FE_FINLINE Vector3F Cross(const Vector3F& other) const noexcept;

        //! \brief Multiply each component of this vector with each component of other vector.
        //! 
        //! \return New vector, this vector is not modified.
        FE_FINLINE Vector3F MulEach(const Vector3F& other) const noexcept;

        //! \brief Check if two vectors are approximately equal.
        //!
        //! \param [in] other   - The vector to compare this vector with.
        //! \param [in] epsilon - Accepted difference between the two vectors.
        //!
        //! \return True if the vectors are approximately equal.
        FE_FINLINE bool IsApproxEqualTo(const Vector3F& other, Float32 epsilon = 0.0001f) const noexcept;

        FE_FINLINE bool operator==(const Vector3F& other) const noexcept;

        FE_FINLINE bool operator!=(const Vector3F& other) const noexcept;

        FE_FINLINE Vector3F operator-() const noexcept;

        FE_FINLINE Vector3F operator+(const Vector3F& other) const noexcept;

        FE_FINLINE Vector3F operator-(const Vector3F& other) const noexcept;

        FE_FINLINE Vector3F operator*(Float32 f) const noexcept;

        FE_FINLINE Vector3F operator/(Float32 f) const noexcept;

        FE_FINLINE Vector3F& operator+=(const Vector3F& other) noexcept;

        FE_FINLINE Vector3F& operator-=(const Vector3F& other) noexcept;

        FE_FINLINE Vector3F& operator*=(Float32 f) noexcept;

        FE_FINLINE Vector3F& operator/=(Float32 f) noexcept;
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

    FE_FINLINE Vector3F::Vector3F(Float32 value) noexcept
        : m_Value(value)
    {
    }

    FE_FINLINE Vector3F::Vector3F(Float32 x, Float32 y, Float32 z) noexcept
        : m_Value(x, y, z)
    {
    }

    FE_FINLINE Vector3F::Vector3F(const std::array<Float32, 3>& array) noexcept
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

    FE_FINLINE Float32 Vector3F::operator[](size_t index) const noexcept
    {
        return m_Values[index];
    }

    FE_FINLINE Float32 Vector3F::operator()(size_t index) const noexcept
    {
        return m_Values[index];
    }

    FE_FINLINE const Float32* Vector3F::Data() const noexcept
    {
        return m_Values;
    }

    FE_FINLINE Vector3F::TVec Vector3F::GetSIMDVector() const noexcept
    {
        return m_Value;
    }

    FE_FINLINE Float32 Vector3F::X() const noexcept
    {
        return m_X;
    }

    FE_FINLINE Float32 Vector3F::Y() const noexcept
    {
        return m_Y;
    }

    FE_FINLINE Float32 Vector3F::Z() const noexcept
    {
        return m_Z;
    }

    FE_FINLINE Float32& Vector3F::X() noexcept
    {
        return m_X;
    }

    FE_FINLINE Float32& Vector3F::Y() noexcept
    {
        return m_Y;
    }

    FE_FINLINE Float32& Vector3F::Z() noexcept
    {
        return m_Z;
    }

    FE_FINLINE void Vector3F::Set(Float32 x, Float32 y, Float32 z) noexcept
    {
        m_Value = TVec(x, y, z);
    }

    FE_FINLINE Float32 Vector3F::Dot(const Vector3F& other) const noexcept
    {
        TVec mul     = m_Value * other.m_Value;
        TVec halfSum = mul.Broadcast<1>() + mul;
        TVec sum     = mul.Broadcast<2>() + halfSum;
        return sum.Select<0>();
    }

    FE_FINLINE Float32 Vector3F::LengthSq() const noexcept
    {
        return Dot(*this);
    }

    FE_FINLINE Float32 Vector3F::Length() const noexcept
    {
        return std::sqrt(LengthSq());
    }

    FE_FINLINE Vector3F Vector3F::Normalized() const noexcept
    {
        Float32 len = Length();
        return m_Value / len;
    }

    FE_FINLINE Vector3F Vector3F::Lerp(const Vector3F& dst, Float32 f) const noexcept
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

    FE_FINLINE bool Vector3F::IsApproxEqualTo(const Vector3F& other, Float32 epsilon) const noexcept
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

    FE_FINLINE Vector3F Vector3F::operator*(Float32 f) const noexcept
    {
        return m_Value * f;
    }

    FE_FINLINE Vector3F Vector3F::operator/(Float32 f) const noexcept
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

    FE_FINLINE Vector3F& Vector3F::operator*=(Float32 f) noexcept
    {
        *this = *this * f;
        return *this;
    }

    FE_FINLINE Vector3F& Vector3F::operator/=(Float32 f) noexcept
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
