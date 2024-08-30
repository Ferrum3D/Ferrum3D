#pragma once
#include <FeCore/Base/BaseMath.h>
#include <FeCore/RTTI/RTTI.h>
#include <FeCore/SIMD/CommonSIMD.h>
#include <array>
#include <cstdint>
#include <iostream>

namespace FE
{
    //! @brief 2-dimensional vector.
    class Vector2F final
    {
        using TVec = SIMD::SSE::Float32x4;

        FE_PUSH_MSVC_WARNING(4201)
        union
        {
            TVec m_Value;
            float m_Values[2];
            struct
            {
                float m_X, m_Y;
            };
        };
        FE_POP_MSVC_WARNING

    public:
        FE_RTTI_Base(Vector2F, "3181CD8D-6109-4E7D-AE6F-E672ED5EDF2C");

        FE_FORCE_INLINE Vector2F()
            : Vector2F(0, 0)
        {
        }

        FE_FORCE_INLINE explicit Vector2F(TVec vec) noexcept;

        FE_FORCE_INLINE Vector2F(const Vector2F& other) noexcept;

        FE_FORCE_INLINE Vector2F& operator=(const Vector2F& other) noexcept;

        FE_FORCE_INLINE Vector2F(Vector2F&& other) noexcept;

        FE_FORCE_INLINE Vector2F& operator=(Vector2F&& other) noexcept;

        FE_FORCE_INLINE explicit Vector2F(float value) noexcept;

        FE_FORCE_INLINE Vector2F(float x, float y) noexcept;

        FE_FORCE_INLINE explicit Vector2F(const std::array<float, 2>& array) noexcept;

        //! @return Vector2F{ 0, 0 }.
        [[nodiscard]] FE_FORCE_INLINE static Vector2F GetZero() noexcept;

        //! @return Vector2F{ 1, 0 }.
        [[nodiscard]] FE_FORCE_INLINE static Vector2F GetUnitX() noexcept;

        //! @return Vector2F{ 0, 1 }.
        [[nodiscard]] FE_FORCE_INLINE static Vector2F GetUnitY() noexcept;

        [[nodiscard]] FE_FORCE_INLINE float operator[](size_t index) const noexcept;

        [[nodiscard]] FE_FORCE_INLINE float operator()(size_t index) const noexcept;

        //! @return A pointer to array of two floats (components of the vector).
        [[nodiscard]] FE_FORCE_INLINE const float* Data() const noexcept;

        //! @return Underlying SIMD type.
        [[nodiscard]] FE_FORCE_INLINE TVec GetSIMD() const noexcept;

        [[nodiscard]] FE_FORCE_INLINE float X() const noexcept;
        [[nodiscard]] FE_FORCE_INLINE float Y() const noexcept;

        [[nodiscard]] FE_FORCE_INLINE float& X() noexcept;
        [[nodiscard]] FE_FORCE_INLINE float& Y() noexcept;

        FE_FORCE_INLINE void Set(float x, float y) noexcept;

        [[nodiscard]] FE_FORCE_INLINE float Dot(const Vector2F& other) const noexcept;

        //! @return Squared length of the vector.
        [[nodiscard]] FE_FORCE_INLINE float LengthSq() const noexcept;

        //! @return Length of the vector.
        [[nodiscard]] FE_FORCE_INLINE float Length() const noexcept;

        //! @return New normalized vector, this vector is not modified.
        [[nodiscard]] FE_FORCE_INLINE Vector2F Normalized() const noexcept;

        //! @brief Linearly interpolate between this and destination.
        //!
        //! The result is (dst - this) * f + this.
        //!
        //! @param f - Interpolation factor.
        //!
        //! @return New interpolated vector, this vector is not modified.
        [[nodiscard]] FE_FORCE_INLINE Vector2F Lerp(const Vector2F& dst, float f) const noexcept;

        //! @brief Multiply each component of this vector with each component of other vector.
        //!
        //! @return New vector, this vector is not modified.
        [[nodiscard]] FE_FORCE_INLINE Vector2F MulEach(const Vector2F& other) const noexcept;

        //! @brief Check if two vectors are approximately equal.
        //!
        //! @param other   - The vector to compare this vector with.
        //! @param epsilon - Accepted difference between the two vectors.
        //!
        //! @return True if the vectors are approximately equal.
        [[nodiscard]] FE_FORCE_INLINE bool IsApproxEqualTo(const Vector2F& other,
                                                           float epsilon = Math::Constants::Epsilon) const noexcept;

        [[nodiscard]] FE_FORCE_INLINE bool operator==(const Vector2F& other) const noexcept;

        [[nodiscard]] FE_FORCE_INLINE bool operator!=(const Vector2F& other) const noexcept;

        [[nodiscard]] FE_FORCE_INLINE Vector2F operator-() const noexcept;

        [[nodiscard]] FE_FORCE_INLINE Vector2F operator+(const Vector2F& other) const noexcept;

        [[nodiscard]] FE_FORCE_INLINE Vector2F operator-(const Vector2F& other) const noexcept;

        [[nodiscard]] FE_FORCE_INLINE Vector2F operator*(float f) const noexcept;

        [[nodiscard]] FE_FORCE_INLINE Vector2F operator/(float f) const noexcept;

        FE_FORCE_INLINE Vector2F& operator+=(const Vector2F& other) noexcept;

        FE_FORCE_INLINE Vector2F& operator-=(const Vector2F& other) noexcept;

        FE_FORCE_INLINE Vector2F& operator*=(float f) noexcept;

        FE_FORCE_INLINE Vector2F& operator/=(float f) noexcept;
    };

    FE_FORCE_INLINE Vector2F::Vector2F(TVec vec) noexcept // NOLINT clang-tidy complains about uninitialized union members
        : m_Value(vec)
    {
    }

    FE_FORCE_INLINE Vector2F::Vector2F(const Vector2F& other) noexcept // NOLINT
        : m_Value(other.m_Value)
    {
    }

    FE_FORCE_INLINE Vector2F& Vector2F::operator=(const Vector2F& other) noexcept
    {
        m_Value = other.m_Value;
        return *this;
    }

    FE_FORCE_INLINE Vector2F::Vector2F(Vector2F&& other) noexcept // NOLINT
        : m_Value(other.m_Value)
    {
    }

    FE_FORCE_INLINE Vector2F& Vector2F::operator=(Vector2F&& other) noexcept
    {
        m_Value = other.m_Value;
        return *this;
    }

    FE_FORCE_INLINE Vector2F::Vector2F(float value) noexcept // NOLINT
        : m_Value(value)
    {
    }

    FE_FORCE_INLINE Vector2F::Vector2F(float x, float y) noexcept // NOLINT
        : m_Value(x, y)
    {
    }

    FE_FORCE_INLINE Vector2F::Vector2F(const std::array<float, 2>& array) noexcept // NOLINT
        : m_Value(array[0], array[1])
    {
    }

    FE_FORCE_INLINE Vector2F Vector2F::GetZero() noexcept
    {
        return Vector2F(0);
    }

    FE_FORCE_INLINE Vector2F Vector2F::GetUnitX() noexcept
    {
        return Vector2F(1, 0);
    }

    FE_FORCE_INLINE Vector2F Vector2F::GetUnitY() noexcept
    {
        return Vector2F(0, 1);
    }

    FE_FORCE_INLINE float Vector2F::operator[](size_t index) const noexcept
    {
        return m_Values[index];
    }

    FE_FORCE_INLINE float Vector2F::operator()(size_t index) const noexcept
    {
        return m_Values[index];
    }

    FE_FORCE_INLINE const float* Vector2F::Data() const noexcept
    {
        return m_Values;
    }

    FE_FORCE_INLINE Vector2F::TVec Vector2F::GetSIMD() const noexcept
    {
        return m_Value;
    }

    FE_FORCE_INLINE float Vector2F::X() const noexcept
    {
        return m_X;
    }

    FE_FORCE_INLINE float Vector2F::Y() const noexcept
    {
        return m_Y;
    }

    FE_FORCE_INLINE float& Vector2F::X() noexcept
    {
        return m_X;
    }

    FE_FORCE_INLINE float& Vector2F::Y() noexcept
    {
        return m_Y;
    }

    FE_FORCE_INLINE void Vector2F::Set(float x, float y) noexcept
    {
        m_Value = TVec(x, y);
    }

    FE_FORCE_INLINE float Vector2F::Dot(const Vector2F& other) const noexcept
    {
        // A scalar version seems to require fewer operations here.
        // TODO: benchmark this
        // TVec mul = m_Value * other.m_Value;
        // return (mul.Broadcast<1>() + mul).Select<0>();
        return m_X * other.m_X + m_Y * other.m_Y;
    }

    FE_FORCE_INLINE float Vector2F::LengthSq() const noexcept
    {
        return Dot(*this);
    }

    FE_FORCE_INLINE float Vector2F::Length() const noexcept
    {
        return std::sqrt(LengthSq());
    }

    FE_FORCE_INLINE Vector2F Vector2F::Normalized() const noexcept
    {
        float len = Length();
        return Vector2F(m_Value / len);
    }

    FE_FORCE_INLINE Vector2F Vector2F::Lerp(const Vector2F& dst, float f) const noexcept
    {
        return Vector2F((dst.m_Value - m_Value) * f + m_Value);
    }

    FE_FORCE_INLINE Vector2F Vector2F::MulEach(const Vector2F& other) const noexcept
    {
        return Vector2F(m_Value * other.m_Value);
    }

    FE_FORCE_INLINE bool Vector2F::IsApproxEqualTo(const Vector2F& other, float epsilon) const noexcept
    {
        return TVec::CompareAllLe((m_Value - other.m_Value).Abs(), epsilon, 0xfff);
    }

    FE_FORCE_INLINE bool Vector2F::operator==(const Vector2F& other) const noexcept
    {
        return TVec::CompareAllEq(m_Value, other.m_Value, 0xfff);
    }

    FE_FORCE_INLINE bool Vector2F::operator!=(const Vector2F& other) const noexcept
    {
        return TVec::CompareAllNeq(m_Value, other.m_Value, 0xfff);
    }

    FE_FORCE_INLINE Vector2F Vector2F::operator-() const noexcept
    {
        return Vector2F(m_Value.Negate());
    }

    FE_FORCE_INLINE Vector2F Vector2F::operator+(const Vector2F& other) const noexcept
    {
        return Vector2F(m_Value + other.m_Value);
    }

    FE_FORCE_INLINE Vector2F Vector2F::operator-(const Vector2F& other) const noexcept
    {
        return Vector2F(m_Value - other.m_Value);
    }

    FE_FORCE_INLINE Vector2F Vector2F::operator*(float f) const noexcept
    {
        return Vector2F(m_Value * f);
    }

    FE_FORCE_INLINE Vector2F Vector2F::operator/(float f) const noexcept
    {
        return Vector2F(m_Value / f);
    }

    FE_FORCE_INLINE Vector2F& Vector2F::operator+=(const Vector2F& other) noexcept
    {
        *this = *this + other;
        return *this;
    }

    FE_FORCE_INLINE Vector2F& Vector2F::operator-=(const Vector2F& other) noexcept
    {
        *this = *this - other;
        return *this;
    }

    FE_FORCE_INLINE Vector2F& Vector2F::operator*=(float f) noexcept
    {
        *this = *this * f;
        return *this;
    }

    FE_FORCE_INLINE Vector2F& Vector2F::operator/=(float f) noexcept
    {
        *this = *this / f;
        return *this;
    }
} // namespace FE
