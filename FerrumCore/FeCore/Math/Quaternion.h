#pragma once
#include <FeCore/Math/Vector3.h>
#include <FeCore/SIMD/CommonSIMD.h>

namespace FE
{
    class Quaternion final
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
        FE_STRUCT_RTTI(Quaternion, "36791C3D-2C83-4516-8359-E0A34E3FC7B9");

        FE_FINLINE Quaternion()
            : Quaternion(TVec::GetZero())
        {
        }

        FE_FINLINE explicit Quaternion(TVec vec) noexcept;

        FE_FINLINE Quaternion(const Quaternion& other) noexcept;

        FE_FINLINE explicit Quaternion(const Vector3F& v, Float32 w) noexcept;

        FE_FINLINE Quaternion& operator=(const Quaternion& other) noexcept;

        FE_FINLINE Quaternion(Quaternion&& other) noexcept;

        FE_FINLINE Quaternion& operator=(Quaternion&& other) noexcept;

        FE_FINLINE explicit Quaternion(Float32 value) noexcept;

        FE_FINLINE Quaternion(Float32 x, Float32 y, Float32 z, Float32 w) noexcept;

        FE_FINLINE explicit Quaternion(const std::array<Float32, 4>& array) noexcept;

        //! \return Quaternion{ 0, 0, 0, 0 }.
        [[nodiscard]] FE_FINLINE static Quaternion GetZero() noexcept;

        //! \return Quaternion{ 0, 0, 0, 1 }.
        [[nodiscard]] FE_FINLINE static Quaternion GetIdentity() noexcept;

        [[nodiscard]] FE_FINLINE static Quaternion CreateRotationX(Float32 angle);
        [[nodiscard]] FE_FINLINE static Quaternion CreateRotationY(Float32 angle);
        [[nodiscard]] FE_FINLINE static Quaternion CreateRotationZ(Float32 angle);

        [[nodiscard]] FE_FINLINE static Quaternion FromAxisAngle(const Vector3F& axis, Float32 angle);
        [[nodiscard]] FE_FINLINE static Quaternion FromEulerAngles(const Vector3F& eulerAngles);
        [[nodiscard]] FE_FINLINE static Quaternion FromEulerAngles(Float32 x, Float32 y, Float32 z);

        FE_FINLINE void GetAxisAngle(Vector3F& axis, Float32& angle) const noexcept;
        [[nodiscard]] FE_FINLINE Vector3F GetEulerAngles() const noexcept;

        [[nodiscard]] FE_FINLINE Float32 operator[](USize index) const noexcept;

        [[nodiscard]] FE_FINLINE Float32& operator()(USize index) noexcept;
        [[nodiscard]] FE_FINLINE Float32 operator()(USize index) const noexcept;

        //! \return A pointer to array of four floats (components of the quaternion).
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

        [[nodiscard]] FE_FINLINE Vector3F Im() const noexcept;

        FE_FINLINE void Set(Float32 x, Float32 y, Float32 z, Float32 w) noexcept;

        [[nodiscard]] FE_FINLINE Float32 Dot(const Quaternion& other) const noexcept;

        //! \return Squared length of the quaternion.
        [[nodiscard]] FE_FINLINE Float32 LengthSq() const noexcept;

        //! \return Length of the quaternion.
        [[nodiscard]] FE_FINLINE Float32 Length() const noexcept;

        [[nodiscard]] FE_FINLINE Quaternion Conjugated() const noexcept;

        [[nodiscard]] FE_FINLINE Quaternion Inverse() const noexcept;

        [[nodiscard]] FE_FINLINE Quaternion Normalized() const noexcept;

        //! \brief Linearly interpolate between this and destination.
        //!
        //! \param [in] f - Interpolation factor.
        //!
        //! \return New interpolated quaternion, this quaternion is not modified.
        [[nodiscard]] FE_FINLINE Quaternion Lerp(const Quaternion& dst, Float32 f) const noexcept;

        //! \brief Spherical linearly interpolate between this and destination.
        //!
        //! \param [in] f - Interpolation factor.
        //!
        //! \return New interpolated quaternion, this quaternion is not modified.
        [[nodiscard]] FE_FINLINE Quaternion SLerp(const Quaternion& dst, Float32 f) const noexcept;

        //! \brief Check if two quaternions are approximately equal.
        //!
        //! \param [in] other   - The quaternion to compare this one with.
        //! \param [in] epsilon - Accepted difference between the two quaternion.
        //!
        //! \return True if the quaternions are approximately equal.
        [[nodiscard]] FE_FINLINE bool IsApproxEqualTo(const Quaternion& other,
                                                      Float32 epsilon = Constants::Epsilon) const noexcept;

        [[nodiscard]] FE_FINLINE bool operator==(const Quaternion& other) const noexcept;
        [[nodiscard]] FE_FINLINE bool operator!=(const Quaternion& other) const noexcept;

        [[nodiscard]] FE_FINLINE Quaternion operator-() const noexcept;

        [[nodiscard]] FE_FINLINE Quaternion operator+(const Quaternion& other) const noexcept;
        [[nodiscard]] FE_FINLINE Quaternion operator-(const Quaternion& other) const noexcept;
        [[nodiscard]] FE_FINLINE Quaternion operator*(const Quaternion& other) const noexcept;
        [[nodiscard]] FE_FINLINE Quaternion operator*(Float32 f) const noexcept;
        [[nodiscard]] FE_FINLINE Quaternion operator/(Float32 f) const noexcept;

        FE_FINLINE Quaternion& operator+=(const Quaternion& other) noexcept;
        FE_FINLINE Quaternion& operator-=(const Quaternion& other) noexcept;
        FE_FINLINE Quaternion& operator*=(const Quaternion& other) noexcept;
        FE_FINLINE Quaternion& operator*=(Float32 f) noexcept;
        FE_FINLINE Quaternion& operator/=(Float32 f) noexcept;
    };

    Quaternion::Quaternion(TVec vec) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init)
        : m_Value(vec)
    {
    }

    Quaternion::Quaternion(const Quaternion& other) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init)
        : m_Value(other.m_Value)
    {
    }

    Quaternion::Quaternion(const Vector3F& v, Float32 w) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init)
        : m_Value(v.GetSIMD())
    {
        m_W = w;
    }

    Quaternion& Quaternion::operator=(const Quaternion& other) noexcept
    {
        m_Value = other.m_Value;
        return *this;
    }

    Quaternion::Quaternion(Quaternion&& other) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init)
    {
        m_Value = other.m_Value;
    }

    Quaternion& Quaternion::operator=(Quaternion&& other) noexcept
    {
        m_Value = other.m_Value;
        return *this;
    }

    Quaternion::Quaternion(Float32 value) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init)
        : m_Value(value)
    {
    }

    Quaternion::Quaternion(Float32 x, Float32 y, Float32 z, Float32 w) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init)
        : m_Value(x, y, z, w)
    {
    }

    Quaternion::Quaternion(const std::array<Float32, 4>& array) noexcept // NOLINT(cppcoreguidelines-pro-type-member-init)
        : m_Value(array[0], array[1], array[2], array[3])
    {
    }

    Quaternion Quaternion::GetZero() noexcept
    {
        return Quaternion(0);
    }

    Quaternion Quaternion::GetIdentity() noexcept
    {
        return { 0, 0, 0, 1 };
    }

    Quaternion Quaternion::CreateRotationX(Float32 angle)
    {
        auto s = std::sin(angle * 0.5f);
        auto c = std::cos(angle * 0.5f);

        return { s, 0, 0, c };
    }

    Quaternion Quaternion::CreateRotationY(Float32 angle)
    {
        auto s = std::sin(angle * 0.5f);
        auto c = std::cos(angle * 0.5f);

        return { 0, s, 0, c };
    }

    Quaternion Quaternion::CreateRotationZ(Float32 angle)
    {
        auto s = std::sin(angle * 0.5f);
        auto c = std::cos(angle * 0.5f);

        return { 0, 0, s, c };
    }

    Quaternion Quaternion::FromAxisAngle(const Vector3F& axis, Float32 angle)
    {
        auto s = std::sin(angle * 0.5f);
        auto c = std::cos(angle * 0.5f);

        return Quaternion(axis * s, c);
    }

    Quaternion Quaternion::FromEulerAngles(const Vector3F& eulerAngles)
    {
        auto angles = eulerAngles * 0.5f;

        auto sx = std::sin(angles.X());
        auto sy = std::sin(angles.Y());
        auto sz = std::sin(angles.Z());
        auto cx = std::cos(angles.X());
        auto cy = std::cos(angles.Y());
        auto cz = std::cos(angles.Z());

        return {
            cx * cy * cz - sx * sy * sz, // x
            cx * sy * sz + sx * cy * cz, // y
            cx * sy * cz - sx * cy * sz, // z
            cx * cy * sz + sx * sy * cz  // w
        };
    }

    Quaternion Quaternion::FromEulerAngles(Float32 x, Float32 y, Float32 z)
    {
        return FromEulerAngles({ x, y, z });
    }

    void Quaternion::GetAxisAngle(Vector3F& axis, Float32& angle) const noexcept
    {
        angle = 2.0f * std::acos(W());

        auto s = std::sin(angle * 0.5f);
        if (s > 0.0f)
        {
            axis = Im() / s;
        }
        else
        {
            axis  = Vector3F::GetUnitY();
            angle = 0.0f;
        }
    }

    Vector3F Quaternion::GetEulerAngles() const noexcept
    {
        auto test = X() * Y() + Z() * W();

        if (test > 0.499f)
        {
            return { 2.0f * std::atan2(X(), W()), Constants::PI * 0.5f, 0 };
        }

        if (test < -0.499)
        {
            return { -2.0f * std::atan2(X(), W()), -Constants::PI / 2, 0 };
        }

        auto sqx = X() * X();
        auto sqy = Y() * Y();
        auto sqz = Z() * Z();
        return { std::atan2(2 * Y() * W() - 2 * X() * Z(), 1 - 2 * sqy - 2 * sqz),
                 std::asin(2 * test),
                 std::atan2(2 * X() * W() - 2 * Y() * Z(), 1 - 2 * sqx - 2 * sqz) };
    }

    Float32 Quaternion::operator[](USize index) const noexcept
    {
        return m_Values[index];
    }

    Float32& Quaternion::operator()(USize index) noexcept
    {
        return m_Values[index];
    }

    Float32 Quaternion::operator()(USize index) const noexcept
    {
        return m_Values[index];
    }

    const Float32* Quaternion::Data() const noexcept
    {
        return m_Values;
    }

    const Quaternion::TVec& Quaternion::GetSIMD() const noexcept
    {
        return m_Value;
    }

    Float32 Quaternion::X() const noexcept
    {
        return m_X;
    }

    Float32 Quaternion::Y() const noexcept
    {
        return m_Y;
    }

    Float32 Quaternion::Z() const noexcept
    {
        return m_Z;
    }

    Float32 Quaternion::W() const noexcept
    {
        return m_W;
    }

    Float32& Quaternion::X() noexcept
    {
        return m_X;
    }

    Float32& Quaternion::Y() noexcept
    {
        return m_Y;
    }

    Float32& Quaternion::Z() noexcept
    {
        return m_Z;
    }

    Float32& Quaternion::W() noexcept
    {
        return m_W;
    }

    Vector3F Quaternion::Im() const noexcept
    {
        return Vector3F(m_Value);
    }

    void Quaternion::Set(Float32 x, Float32 y, Float32 z, Float32 w) noexcept
    {
        m_Value = TVec(x, y, z, w);
    }

    Float32 Quaternion::Dot(const Quaternion& other) const noexcept
    {
        TVec mul = m_Value * other.m_Value;
        TVec t   = mul * mul.Shuffle<2, 3, 0, 1>();
        TVec r   = t + t.Shuffle<1, 0, 2, 3>();
        return r.Select<0>();
    }

    Float32 Quaternion::LengthSq() const noexcept
    {
        return Dot(*this);
    }

    Float32 Quaternion::Length() const noexcept
    {
        return std::sqrt(LengthSq());
    }

    Quaternion Quaternion::Conjugated() const noexcept
    {
        return Quaternion(m_Value.NegateXYZ());
    }

    Quaternion Quaternion::Inverse() const noexcept
    {
        return Conjugated() / LengthSq();
    }

    Quaternion Quaternion::Normalized() const noexcept
    {
        Float32 len = Length();
        return Quaternion(m_Value / len);
    }

    Quaternion Quaternion::Lerp(const Quaternion& dst, Float32 f) const noexcept
    {
        if (Dot(dst) >= 0.0f)
        {
            return *this * (1.0f - f) + dst * f;
        }

        return *this * (1.0f - f) - dst * f;
    }

    Quaternion Quaternion::SLerp(const Quaternion& dst, Float32 f) const noexcept
    {
        auto cosHalfTheta = W() * dst.W() + X() * dst.X() + Y() * dst.Y() + Z() * dst.Z();

        if (abs(cosHalfTheta) >= 1.0)
        {
            return *this;
        }

        Float32 halfTheta    = std::acos(cosHalfTheta);
        Float32 sinHalfTheta = std::sqrt(1.0f - cosHalfTheta * cosHalfTheta);

        if (std::abs(sinHalfTheta) < 0.001f)
        {
            return Quaternion(m_Value * 0.5f + dst.m_Value * 0.5f);
        }

        Float32 ratioA = std::sin((1.0f - f) * halfTheta) / sinHalfTheta;
        Float32 ratioB = std::sin(f * halfTheta) / sinHalfTheta;

        return Quaternion(m_Value * ratioA + dst.m_Value * ratioB);
    }

    bool Quaternion::IsApproxEqualTo(const Quaternion& other, Float32 epsilon) const noexcept
    {
        return TVec::CompareAllLe((m_Value - other.m_Value).Abs(), epsilon, 0xffff);
    }

    bool Quaternion::operator==(const Quaternion& other) const noexcept
    {
        return TVec::CompareAllEq(m_Value, other.m_Value, 0xffff);
    }

    bool Quaternion::operator!=(const Quaternion& other) const noexcept
    {
        return TVec::CompareAllNeq(m_Value, other.m_Value, 0xffff);
    }

    Quaternion Quaternion::operator-() const noexcept
    {
        return Quaternion(0.0f - m_Value);
    }

    Quaternion Quaternion::operator+(const Quaternion& other) const noexcept
    {
        return Quaternion(m_Value + other.m_Value);
    }

    Quaternion Quaternion::operator-(const Quaternion& other) const noexcept
    {
        return Quaternion(m_Value - other.m_Value);
    }

    Quaternion Quaternion::operator*(const Quaternion& other) const noexcept
    {
        auto a1123 = m_Value.Shuffle<3, 2, 1, 1>();
        auto a2231 = m_Value.Shuffle<1, 3, 2, 2>();
        auto b1000 = other.m_Value.Shuffle<0, 0, 0, 1>();
        auto b2312 = other.m_Value.Shuffle<2, 1, 3, 2>();

        auto t1 = a1123 * b1000;
        auto t2 = a2231 * b2312;

        auto t12 = (t1 * t2).NegateW();

        auto a3312 = m_Value.Shuffle<2, 1, 3, 3>();
        auto b3231 = other.m_Value.Shuffle<1, 3, 2, 3>();
        auto a0000 = m_Value.Broadcast<0>();

        auto t3  = a3312 * b3231;
        auto t0  = a0000 * other.m_Value;
        auto t03 = t0 - t3;
        return Quaternion(t03 + t12);
    }

    Quaternion Quaternion::operator*(Float32 f) const noexcept
    {
        return Quaternion(m_Value * f);
    }

    Quaternion Quaternion::operator/(Float32 f) const noexcept
    {
        return Quaternion(m_Value / f);
    }

    Quaternion& Quaternion::operator+=(const Quaternion& other) noexcept
    {
        *this = *this + other;
        return *this;
    }

    Quaternion& Quaternion::operator-=(const Quaternion& other) noexcept
    {
        *this = *this - other;
        return *this;
    }

    Quaternion& Quaternion::operator*=(const Quaternion& other) noexcept
    {
        *this = *this * other;
        return *this;
    }

    Quaternion& Quaternion::operator*=(Float32 f) noexcept
    {
        *this = *this * f;
        return *this;
    }

    Quaternion& Quaternion::operator/=(Float32 f) noexcept
    {
        *this = *this / f;
        return *this;
    }
} // namespace FE

namespace std // NOLINT
{
    inline ostream& operator<<(ostream& stream, const FE::Quaternion& vec)
    {
        return stream << "{ (" << vec.X() << "i + " << vec.Y() << "j + " << vec.Z() << "k) + " << vec.W() << " }";
    }
} // namespace std
