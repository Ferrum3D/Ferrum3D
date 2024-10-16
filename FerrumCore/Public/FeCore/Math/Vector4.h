#pragma once
#include <FeCore/Math/Vector3.h>
#include <FeCore/SIMD/CommonSIMD.h>
#include <array>
#include <cstdint>
#include <iostream>

namespace FE
{
    //! @brief 4-dimensional vector.
    class Vector4F final
    {
        using TVec = SIMD::SSE::Float32x4;

        FE_PUSH_MSVC_WARNING(4201)
        union
        {
            TVec m_Value;
            float m_Values[4];
            struct
            {
                float m_X, m_Y, m_Z, m_W;
            };
        };
        FE_POP_MSVC_WARNING

    public:
        FE_RTTI_Base(Vector4F, "C8B28F38-DAAB-4C9A-A922-41E881ED184C");

        FE_FORCE_INLINE Vector4F()
            : Vector4F(0, 0, 0, 0)
        {
        }

        FE_FORCE_INLINE explicit Vector4F(TVec vec) noexcept;

        FE_FORCE_INLINE Vector4F(const Vector4F& other) noexcept;

        FE_FORCE_INLINE explicit Vector4F(const Vector3F& other, float w = 1.0f) noexcept;

        FE_FORCE_INLINE Vector4F& operator=(const Vector4F& other) noexcept;

        FE_FORCE_INLINE Vector4F(Vector4F&& other) noexcept;

        FE_FORCE_INLINE Vector4F& operator=(Vector4F&& other) noexcept;

        FE_FORCE_INLINE explicit Vector4F(float value) noexcept;

        FE_FORCE_INLINE Vector4F(float x, float y, float z, float w) noexcept;

        [[nodiscard]] FE_FORCE_INLINE Vector3F GetVector3F() const noexcept;

        FE_FORCE_INLINE explicit Vector4F(const std::array<float, 4>& array) noexcept;

        //! @return Vector4F{ 0, 0, 0, 0 }.
        [[nodiscard]] FE_FORCE_INLINE static Vector4F GetZero() noexcept;

        //! @return Vector4F{ 1, 0, 0, 0 }.
        [[nodiscard]] FE_FORCE_INLINE static Vector4F GetUnitX() noexcept;

        //! @return Vector4F{ 0, 1, 0, 0 }.
        [[nodiscard]] FE_FORCE_INLINE static Vector4F GetUnitY() noexcept;

        //! @return Vector4F{ 0, 0, 1, 0 }.
        [[nodiscard]] FE_FORCE_INLINE static Vector4F GetUnitZ() noexcept;

        //! @return Vector4F{ 0, 0, 0, 1 }.
        [[nodiscard]] FE_FORCE_INLINE static Vector4F GetUnitW() noexcept;

        [[nodiscard]] FE_FORCE_INLINE float operator[](size_t index) const noexcept;

        [[nodiscard]] FE_FORCE_INLINE float& operator()(size_t index) noexcept;
        [[nodiscard]] FE_FORCE_INLINE float operator()(size_t index) const noexcept;

        //! @return A pointer to array of four floats (components of the vector).
        [[nodiscard]] FE_FORCE_INLINE const float* Data() const noexcept;

        //! @return Underlying SIMD type.
        [[nodiscard]] FE_FORCE_INLINE const TVec& GetSIMD() const noexcept;

        [[nodiscard]] FE_FORCE_INLINE float X() const noexcept;
        [[nodiscard]] FE_FORCE_INLINE float Y() const noexcept;
        [[nodiscard]] FE_FORCE_INLINE float Z() const noexcept;
        [[nodiscard]] FE_FORCE_INLINE float W() const noexcept;

        [[nodiscard]] FE_FORCE_INLINE float& X() noexcept;
        [[nodiscard]] FE_FORCE_INLINE float& Y() noexcept;
        [[nodiscard]] FE_FORCE_INLINE float& Z() noexcept;
        [[nodiscard]] FE_FORCE_INLINE float& W() noexcept;

        FE_FORCE_INLINE void Set(float x, float y, float z, float w) noexcept;

        [[nodiscard]] FE_FORCE_INLINE float Dot(const Vector4F& other) const noexcept;

        //! @return Squared length of the vector.
        [[nodiscard]] FE_FORCE_INLINE float LengthSq() const noexcept;

        //! @return Length of the vector.
        [[nodiscard]] FE_FORCE_INLINE float Length() const noexcept;

        //! @return New normalized vector, this vector is not modified.
        [[nodiscard]] FE_FORCE_INLINE Vector4F Normalized() const noexcept;

        //! @brief Linearly interpolate between this and destination.
        //!
        //! The result is (dst - this) * f + this;
        //!
        //! @param f - Interpolation factor.
        //!
        //! @return New interpolated vector, this vector is not modified.
        [[nodiscard]] FE_FORCE_INLINE Vector4F Lerp(const Vector4F& dst, float f) const noexcept;

        //! @brief Multiply each component of this vector with each component of other vector.
        //! @return New vector, this vector is not modified.
        [[nodiscard]] FE_FORCE_INLINE Vector4F MulEach(const Vector4F& other) const noexcept;

        //! @brief Check if two vectors are approximately equal.
        //!
        //! @param other   - The vector to compare this vector with.
        //! @param epsilon - Accepted difference between the two vectors.
        //!
        //! @return True if the vectors are approximately equal.
        [[nodiscard]] FE_FORCE_INLINE bool IsApproxEqualTo(const Vector4F& other, float epsilon = Constants::Epsilon) const noexcept;

        [[nodiscard]] FE_FORCE_INLINE bool operator==(const Vector4F& other) const noexcept;
        [[nodiscard]] FE_FORCE_INLINE bool operator!=(const Vector4F& other) const noexcept;

        [[nodiscard]] FE_FORCE_INLINE Vector4F operator-() const noexcept;

        [[nodiscard]] FE_FORCE_INLINE Vector4F operator+(const Vector4F& other) const noexcept;
        [[nodiscard]] FE_FORCE_INLINE Vector4F operator-(const Vector4F& other) const noexcept;
        [[nodiscard]] FE_FORCE_INLINE Vector4F operator*(float f) const noexcept;
        [[nodiscard]] FE_FORCE_INLINE Vector4F operator/(float f) const noexcept;

        FE_FORCE_INLINE Vector4F& operator+=(const Vector4F& other) noexcept;
        FE_FORCE_INLINE Vector4F& operator-=(const Vector4F& other) noexcept;
        FE_FORCE_INLINE Vector4F& operator*=(float f) noexcept;
        FE_FORCE_INLINE Vector4F& operator/=(float f) noexcept;
    };

    Vector4F::Vector4F(TVec vec) noexcept // NOLINT clang-tidy complains about uninitialized union members
        : m_Value(vec)
    {
    }

    Vector4F::Vector4F(const Vector4F& other) noexcept // NOLINT
        : m_Value(other.m_Value)
    {
    }

    Vector4F::Vector4F(const Vector3F& other, float w) noexcept // NOLINT
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

    Vector4F::Vector4F(float value) noexcept // NOLINT
        : m_Value(value)
    {
    }

    Vector4F::Vector4F(float x, float y, float z, float w) noexcept // NOLINT
        : m_Value(x, y, z, w)
    {
    }

    Vector4F::Vector4F(const std::array<float, 4>& array) noexcept // NOLINT
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

    float Vector4F::operator[](size_t index) const noexcept
    {
        return m_Values[index];
    }

    float Vector4F::operator()(size_t index) const noexcept
    {
        return m_Values[index];
    }

    float& Vector4F::operator()(size_t index) noexcept
    {
        return m_Values[index];
    }

    const float* Vector4F::Data() const noexcept
    {
        return m_Values;
    }

    const Vector4F::TVec& Vector4F::GetSIMD() const noexcept
    {
        return m_Value;
    }

    float Vector4F::X() const noexcept
    {
        return m_X;
    }

    float Vector4F::Y() const noexcept
    {
        return m_Y;
    }

    float Vector4F::Z() const noexcept
    {
        return m_Z;
    }

    float Vector4F::W() const noexcept
    {
        return m_W;
    }

    float& Vector4F::X() noexcept
    {
        return m_X;
    }

    float& Vector4F::Y() noexcept
    {
        return m_Y;
    }

    float& Vector4F::Z() noexcept
    {
        return m_Z;
    }

    float& Vector4F::W() noexcept
    {
        return m_W;
    }

    void Vector4F::Set(float x, float y, float z, float w) noexcept
    {
        m_Value = TVec(x, y, z, w);
    }

    float Vector4F::Dot(const Vector4F& other) const noexcept
    {
        TVec mul = m_Value * other.m_Value;
        TVec t   = mul * mul.Shuffle<2, 3, 0, 1>();
        TVec r   = t + t.Shuffle<1, 0, 2, 3>();
        return r.Select<0>();
    }

    float Vector4F::LengthSq() const noexcept
    {
        return Dot(*this);
    }

    float Vector4F::Length() const noexcept
    {
        return std::sqrt(LengthSq());
    }

    Vector4F Vector4F::Normalized() const noexcept
    {
        float len = Length();
        return Vector4F(m_Value / len);
    }

    Vector4F Vector4F::Lerp(const Vector4F& dst, float f) const noexcept
    {
        return Vector4F((dst.m_Value - m_Value) * f + m_Value);
    }

    Vector4F Vector4F::MulEach(const Vector4F& other) const noexcept
    {
        return Vector4F(m_Value * other.m_Value);
    }

    bool Vector4F::IsApproxEqualTo(const Vector4F& other, float epsilon) const noexcept
    {
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
        return Vector4F(m_Value.Negate());
    }

    Vector4F Vector4F::operator+(const Vector4F& other) const noexcept
    {
        return Vector4F(m_Value + other.m_Value);
    }

    Vector4F Vector4F::operator-(const Vector4F& other) const noexcept
    {
        return Vector4F(m_Value - other.m_Value);
    }

    Vector4F Vector4F::operator*(float f) const noexcept
    {
        return Vector4F(m_Value * f);
    }

    Vector4F Vector4F::operator/(float f) const noexcept
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

    Vector4F& Vector4F::operator*=(float f) noexcept
    {
        *this = *this * f;
        return *this;
    }

    Vector4F& Vector4F::operator/=(float f) noexcept
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
