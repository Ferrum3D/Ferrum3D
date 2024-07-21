#pragma once
#include <FeCore/Math/Vector4.h>

namespace FE
{
    class Color
    {
        Vector4F m_Color;
        inline static constexpr float MaxByte    = 255.0f;
        inline static constexpr float InvMaxByte = 1.0f / 255.0f;

    public:
        FE_STRUCT_RTTI(Color, "EC4C20BD-05B1-4F3F-B370-D8AE749A3C4F");

        FE_FINLINE Color() noexcept;

        FE_FINLINE explicit Color(const Vector4F& vector);

        FE_FINLINE explicit Color(const Vector3F& rgb, float a = 1.0f);

        FE_FINLINE explicit Color(float rgba);

        FE_FINLINE Color(float r, float g, float b, float a);

        [[nodiscard]] FE_FINLINE static Color GetZero();

        [[nodiscard]] FE_FINLINE static Color FromBytes(UInt8 r, UInt8 g, UInt8 b, UInt8 a) noexcept;
        [[nodiscard]] FE_FINLINE static Color FromUInt32(UInt32 rgba);

        [[nodiscard]] FE_FINLINE float operator[](size_t index) const noexcept;

        [[nodiscard]] FE_FINLINE float operator()(size_t index) const noexcept;

        [[nodiscard]] FE_FINLINE const float* Data() const noexcept;

        [[nodiscard]] FE_FINLINE Vector4F GetVector4F() const noexcept;
        [[nodiscard]] FE_FINLINE Vector3F GetVector3F() const noexcept;

        [[nodiscard]] FE_FINLINE float R32() const noexcept;
        [[nodiscard]] FE_FINLINE float G32() const noexcept;
        [[nodiscard]] FE_FINLINE float B32() const noexcept;
        [[nodiscard]] FE_FINLINE float A32() const noexcept;

        [[nodiscard]] FE_FINLINE float& R32() noexcept;
        [[nodiscard]] FE_FINLINE float& G32() noexcept;
        [[nodiscard]] FE_FINLINE float& B32() noexcept;
        [[nodiscard]] FE_FINLINE float& A32() noexcept;

        [[nodiscard]] FE_FINLINE UInt8 R8() const noexcept;
        [[nodiscard]] FE_FINLINE UInt8 G8() const noexcept;
        [[nodiscard]] FE_FINLINE UInt8 B8() const noexcept;
        [[nodiscard]] FE_FINLINE UInt8 A8() const noexcept;

        [[nodiscard]] FE_FINLINE Color ToLinear() const noexcept;
        [[nodiscard]] FE_FINLINE Color ToSRGB() const noexcept;

        [[nodiscard]] FE_FINLINE Color ToLinearApprox() const noexcept;
        [[nodiscard]] FE_FINLINE Color ToSRGBApprox() const noexcept;

        [[nodiscard]] FE_FINLINE UInt32 ToUInt32() const noexcept;

        [[nodiscard]] FE_FINLINE Color Lerp(const Color& dst, float f) const noexcept;

        [[nodiscard]] FE_FINLINE Color MulEach(const Color& color) const noexcept;

        [[nodiscard]] FE_FINLINE bool IsApproxEqualTo(const Color& other, float epsilon = 0.0001f) const noexcept;

        [[nodiscard]] FE_FINLINE bool operator==(const Color& other) const noexcept;

        [[nodiscard]] FE_FINLINE bool operator!=(const Color& other) const noexcept;

        [[nodiscard]] FE_FINLINE Color operator-() const noexcept;

        [[nodiscard]] FE_FINLINE Color operator+(const Color& other) const noexcept;

        [[nodiscard]] FE_FINLINE Color operator-(const Color& other) const noexcept;

        [[nodiscard]] FE_FINLINE Color operator*(float f) const noexcept;

        [[nodiscard]] FE_FINLINE Color operator/(float f) const noexcept;

        FE_FINLINE Color& operator+=(const Color& other) noexcept;

        FE_FINLINE Color& operator-=(const Color& other) noexcept;

        FE_FINLINE Color& operator*=(float f) noexcept;

        FE_FINLINE Color& operator/=(float f) noexcept;
    };

    FE_FINLINE Color::Color() noexcept
        : m_Color()
    {
    }

    FE_FINLINE Color::Color(const Vector4F& vector)
        : m_Color(vector)
    {
    }

    FE_FINLINE Color::Color(const Vector3F& rgb, float a)
        : m_Color(rgb, a)
    {
    }

    FE_FINLINE Color::Color(float rgba)
        : m_Color(rgba)
    {
    }

    FE_FINLINE Color::Color(float r, float g, float b, float a)
        : m_Color(r, g, b, a)
    {
    }

    FE_FINLINE Color Color::GetZero()
    {
        return Color(0);
    }

    FE_FINLINE Color Color::FromBytes(UInt8 r, UInt8 g, UInt8 b, UInt8 a) noexcept
    {
        auto red   = static_cast<float>(r) * InvMaxByte;
        auto green = static_cast<float>(g) * InvMaxByte;
        auto blue  = static_cast<float>(b) * InvMaxByte;
        auto alpha = static_cast<float>(a) * InvMaxByte;

        return Color(red, green, blue, alpha);
    }

    FE_FINLINE Color Color::FromUInt32(UInt32 rgba)
    {
        auto r = (static_cast<float>((rgba >> 24) & 0xFF) * InvMaxByte);
        auto g = (static_cast<float>((rgba >> 16) & 0xFF) * InvMaxByte);
        auto b = (static_cast<float>((rgba >> 8) & 0xFF) * InvMaxByte);
        auto a = (static_cast<float>((rgba >> 0) & 0xFF) * InvMaxByte);

        return Color(r, g, b, a);
    }

    FE_FINLINE float Color::operator[](size_t index) const noexcept
    {
        return m_Color[index];
    }

    FE_FINLINE float Color::operator()(size_t index) const noexcept
    {
        return m_Color(index);
    }

    FE_FINLINE const float* Color::Data() const noexcept
    {
        return m_Color.Data();
    }

    FE_FINLINE Vector4F Color::GetVector4F() const noexcept
    {
        return m_Color;
    }

    FE_FINLINE Vector3F Color::GetVector3F() const noexcept
    {
        return m_Color.GetVector3F();
    }

    FE_FINLINE float Color::R32() const noexcept
    {
        return m_Color.X();
    }

    FE_FINLINE float Color::G32() const noexcept
    {
        return m_Color.Y();
    }

    FE_FINLINE float Color::B32() const noexcept
    {
        return m_Color.Z();
    }

    FE_FINLINE float Color::A32() const noexcept
    {
        return m_Color.W();
    }

    FE_FINLINE float& Color::R32() noexcept
    {
        return m_Color.X();
    }

    FE_FINLINE float& Color::G32() noexcept
    {
        return m_Color.Y();
    }

    FE_FINLINE float& Color::B32() noexcept
    {
        return m_Color.Z();
    }

    FE_FINLINE float& Color::A32() noexcept
    {
        return m_Color.W();
    }

    FE_FINLINE UInt8 Color::R8() const noexcept
    {
        return static_cast<UInt8>(MaxByte * m_Color.X());
    }

    FE_FINLINE UInt8 Color::G8() const noexcept
    {
        return static_cast<UInt8>(MaxByte * m_Color.Y());
    }

    FE_FINLINE UInt8 Color::B8() const noexcept
    {
        return static_cast<UInt8>(MaxByte * m_Color.Z());
    }

    FE_FINLINE UInt8 Color::A8() const noexcept
    {
        return static_cast<UInt8>(MaxByte * m_Color.W());
    }

    FE_FINLINE Color Color::ToLinear() const noexcept
    {
        auto toLinear = [](float v) {
            return v <= 0.04045f ? v / 12.92f : powf((v + 0.055f) / 1.055f, 2.4f);
        };
        return Color(toLinear(R32()), toLinear(G32()), toLinear(B32()), toLinear(A32()));
    }

    FE_FINLINE Color Color::ToSRGB() const noexcept
    {
        auto toSRGB = [](float v) {
            return v <= 0.0031308f ? v * 12.92f : powf(v, 1.0f / 2.4f) * 1.055f - 0.055f;
        };
        return Color(toSRGB(R32()), toSRGB(G32()), toSRGB(B32()), toSRGB(A32()));
    }

    FE_FINLINE Color Color::ToLinearApprox() const noexcept
    {
        auto toLinear = [](float v) {
            return powf(v, 2.2f);
        };
        return Color(toLinear(R32()), toLinear(G32()), toLinear(B32()), toLinear(A32()));
    }

    FE_FINLINE Color Color::ToSRGBApprox() const noexcept
    {
        auto toSRGB = [](float v) {
            return powf(v, 1.0f / 2.2f);
        };
        return Color(toSRGB(R32()), toSRGB(G32()), toSRGB(B32()), toSRGB(A32()));
    }

    FE_FINLINE UInt32 Color::ToUInt32() const noexcept
    {
        return (A8() << 24) | (B8() << 16) | (G8() << 8) | R8();
    }

    FE_FINLINE Color Color::Lerp(const Color& dst, float f) const noexcept
    {
        return Color(m_Color.Lerp(dst.m_Color, f));
    }

    FE_FINLINE Color Color::MulEach(const Color& color) const noexcept
    {
        return Color(m_Color.MulEach(color.m_Color));
    }

    FE_FINLINE bool Color::IsApproxEqualTo(const Color& other, float epsilon) const noexcept
    {
        return m_Color.IsApproxEqualTo(other.m_Color, epsilon);
    }

    FE_FINLINE bool Color::operator==(const Color& other) const noexcept
    {
        return m_Color == other.m_Color;
    }

    FE_FINLINE bool Color::operator!=(const Color& other) const noexcept
    {
        return m_Color != other.m_Color;
    }

    FE_FINLINE Color Color::operator-() const noexcept
    {
        return Color(-m_Color);
    }

    FE_FINLINE Color Color::operator+(const Color& other) const noexcept
    {
        return Color(m_Color + other.m_Color);
    }

    FE_FINLINE Color Color::operator-(const Color& other) const noexcept
    {
        return Color(m_Color - other.m_Color);
    }

    FE_FINLINE Color Color::operator*(float f) const noexcept
    {
        return Color(m_Color * f);
    }

    FE_FINLINE Color Color::operator/(float f) const noexcept
    {
        return Color(m_Color / f);
    }

    FE_FINLINE Color& Color::operator+=(const Color& other) noexcept
    {
        m_Color += other.m_Color;
        return *this;
    }

    FE_FINLINE Color& Color::operator-=(const Color& other) noexcept
    {
        m_Color -= other.m_Color;
        return *this;
    }

    FE_FINLINE Color& Color::operator*=(float f) noexcept
    {
        m_Color *= f;
        return *this;
    }

    FE_FINLINE Color& Color::operator/=(float f) noexcept
    {
        m_Color /= f;
        return *this;
    }
} // namespace FE
