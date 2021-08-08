#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/RTTI/RTTI.h>
#include <iostream>
#include <limits>
#include <stdint.h>

namespace FE
{
    namespace FeHalfConst
    {
        inline constexpr Float32 Denorm     = 1.0f / 16384.0f;
        inline constexpr Int32 BitsMantissa = 10;
        inline constexpr Int32 BitsExponent = 5;
        inline constexpr Int32 MaxExpVal    = 0x1f;
        inline constexpr Int32 Bias         = MaxExpVal / 2;
        inline constexpr Int32 MaxExp       = +Bias;
        inline constexpr Int32 MinExp       = -Bias;

        inline constexpr Int32 DoubleBitsMantissa = 52;
        inline constexpr Int32 DoubleBitsExponent = 11;
        inline constexpr Int32 DoubleMaxExpVal    = 0x7ff;
        inline constexpr Int32 DoubleBias         = DoubleMaxExpVal / 2;

        inline constexpr Int32 FloatBitsMantissa = 23;
        inline constexpr Int32 FloatBitsExponent = 8;
        inline constexpr Int32 FloatMaxExpVal    = 0xff;
        inline constexpr Int32 FloatBias         = FloatMaxExpVal / 2;
    } // namespace FeHalfConst

    union HalfFormat
    {
        UInt16 Data;

        struct
        {
            UInt16 M : FeHalfConst::BitsMantissa;
            UInt16 E : FeHalfConst::BitsExponent;
            UInt16 S : 1;
        } SEM; // Sign-Exponent-Mantissa

        constexpr HalfFormat(UInt16 b)
            : Data(b)
        {
        }
    };

    union FloatFormat
    {
        Float32 Data;

        struct
        {
            UInt32 M : FeHalfConst::FloatBitsMantissa;
            UInt32 E : FeHalfConst::FloatBitsExponent;
            UInt32 S : 1;
        } SEM; // Sign-Exponent-Mantissa
    };

    union DoubleFormat
    {
        Float64 Data;

        struct
        {
            UInt64 M : FeHalfConst::DoubleBitsMantissa;
            UInt64 E : FeHalfConst::DoubleBitsExponent;
            UInt64 S : 1;
        } SEM; // Sign-Exponent-Mantissa
    };

    /**
    * @brief Half-precision floating point.
    */
    struct FeHalf
    {
        FE_STRUCT_RTTI(FeHalf, "F6FB0AF8-5F42-4C0B-97D3-70079F094924");

        constexpr FeHalf() noexcept
            : m_data(0)
        {
        }

        constexpr FeHalf(int iVal) noexcept
            : FeHalf(Float32(iVal))
        {
        }

        constexpr FeHalf(Float32 fVal) noexcept
            : m_data(0)
        {
            FloatFormat f{};
            f.Data = fVal;

            m_data.SEM.S = f.SEM.S; // copy sign

            // Zero
            if (f.SEM.E == 0)
            {
                m_data.SEM.M = 0;
                m_data.SEM.E = 0;
            }
            // All ones in Exponent: either infinity or NaN
            else if (f.SEM.E == FeHalfConst::FloatMaxExpVal)
            {
                if (f.SEM.M != 0)
                { // NaN
                    m_data.SEM.M = 1;
                }
                else
                { // infinity
                    m_data.SEM.M = 0;
                }
                m_data.SEM.E = FeHalfConst::MaxExpVal;
            }
            // A normal number (not infinity, zero or NaN)
            else
            {
                Int32 exp = f.SEM.E - 127;

                if (exp < -24)
                { // Zero in half precision
                    m_data.SEM.M = 0;
                    m_data.SEM.E = 0;
                }
                else if (exp < -14)
                { // Denorm
                    m_data.SEM.E = 0;

                    UInt32 exp1 = (UInt32)(-14 - exp);
                    switch (exp1)
                    {
                    case 0:
                        m_data.SEM.M = 0;
                        break;
                    case 1:
                        m_data.SEM.M = 512 + (f.SEM.M >> 14);
                        break;
                    case 2:
                        m_data.SEM.M = 256 + (f.SEM.M >> 15);
                        break;
                    case 3:
                        m_data.SEM.M = 128 + (f.SEM.M >> 16);
                        break;
                    case 4:
                        m_data.SEM.M = 64 + (f.SEM.M >> 17);
                        break;
                    case 5:
                        m_data.SEM.M = 32 + (f.SEM.M >> 18);
                        break;
                    case 6:
                        m_data.SEM.M = 16 + (f.SEM.M >> 19);
                        break;
                    case 7:
                        m_data.SEM.M = 8 + (f.SEM.M >> 20);
                        break;
                    case 8:
                        m_data.SEM.M = 4 + (f.SEM.M >> 21);
                        break;
                    case 9:
                        m_data.SEM.M = 2 + (f.SEM.M >> 22);
                        break;
                    case 10:
                        m_data.SEM.M = 1;
                        break;
                    default:
                        break;
                    }
                }
                else if (exp > 15)
                { // Infinity in half precision
                    m_data.SEM.M = 0;
                    m_data.SEM.E = FeHalfConst::MaxExpVal;
                }
                else
                {
                    m_data.SEM.E = exp + 15;
                    m_data.SEM.M = f.SEM.M >> 13;
                }
            }
        }

        constexpr FeHalf(Float64 dVal) noexcept
            : FeHalf(Float32(dVal))
        {
        }

        constexpr FeHalf(const FeHalf& other) noexcept
            : m_data(other.m_data)
        {
        }
        constexpr FeHalf(FeHalf&& other) noexcept
            : m_data(other.m_data)
        {
        }

        constexpr FeHalf& operator=(const FeHalf& other) noexcept
        {
            m_data = other.m_data;
            return *this;
        }

        constexpr FeHalf& operator=(FeHalf&& other) noexcept
        {
            m_data = other.m_data;
            return *this;
        }

        constexpr FeHalf& operator=(Float32 other) noexcept
        {
            *this = FeHalf(other);
            return *this;
        }

        constexpr FeHalf& operator=(Float64 other) noexcept
        {
            *this = FeHalf(other);
            return *this;
        }

        constexpr operator Float32() const noexcept
        {
            FloatFormat f{};
            f.SEM.S = m_data.SEM.S;

            if (m_data.SEM.E == 0)
            {
                if (m_data.SEM.M == 0)
                {
                    f.SEM.M = 0;
                    f.SEM.E = 0;
                }
                else
                {
                    Float32 mantissa = Float32(m_data.SEM.M) / 1024.0f;
                    Float32 sign     = (m_data.SEM.S) ? -1.0f : 1.0f;
                    f.Data           = sign * mantissa * FeHalfConst::Denorm;
                }
            }
            else if (m_data.SEM.E == FeHalfConst::MaxExpVal)
            {
                f.SEM.E = FeHalfConst::FloatMaxExpVal;
                f.SEM.M = (m_data.SEM.M != 0) ? 1 : 0;
            }
            else
            {
                f.SEM.E = m_data.SEM.E + 112;
                f.SEM.M = m_data.SEM.M << 13;
            }
            return f.Data;
        }

        constexpr operator Float64() const noexcept
        {
            return Float64(Float32(*this));
        }

        constexpr bool operator!=(FeHalf other) const noexcept
        {
            // Both are zeros: sign doesn't matter
            if ((m_data.Data & 0x7f'ff) == 0 && (other.m_data.Data & 0x7f'ff) == 0)
            {
                return false;
            }

            return m_data.Data != other.m_data.Data || IsNaN();
        }

        constexpr bool operator==(FeHalf other) const noexcept
        {
            // Both are zeros: sign doesn't matter
            if ((m_data.Data & 0x7f'ff) == 0 && (other.m_data.Data & 0x7f'ff) == 0)
            {
                return true;
            }

            return m_data.Data == other.m_data.Data && !IsNaN();
        }

        constexpr bool operator==(Float32 other) const noexcept
        {
            return Float32(*this) == other;
        }

        constexpr bool operator==(Float64 other) const noexcept
        {
            return Float64(*this) == other;
        }

        constexpr bool operator==(int other) const noexcept
        {
            return Float32(*this) == other;
        }

        constexpr bool operator<(FeHalf other) const noexcept
        {
            if (IsNaN() || other.IsNaN())
            {
                return false;
            }
            return m_data.Data < other.m_data.Data;
        }

        constexpr bool operator>(FeHalf other) const noexcept
        {
            if (IsNaN() || other.IsNaN())
            {
                return false;
            }
            return m_data.Data > other.m_data.Data;
        }

        constexpr bool operator<=(FeHalf other) const noexcept
        {
            if (IsNaN() || other.IsNaN())
            {
                return false;
            }
            return !(*this > other);
        }

        constexpr bool operator>=(FeHalf other) const noexcept
        {
            if (IsNaN() || other.IsNaN())
            {
                return false;
            }
            return !(*this < other);
        }

        constexpr FeHalf& operator+=(FeHalf other) noexcept;
        constexpr FeHalf& operator-=(FeHalf other) noexcept;
        constexpr FeHalf& operator*=(FeHalf other) noexcept;
        constexpr FeHalf& operator/=(FeHalf other) noexcept;

        constexpr FeHalf operator-() const noexcept
        {
            return FeHalf(~m_data.SEM.S, m_data.SEM.E, m_data.SEM.M);
        }

        constexpr bool IsNaN() const
        {
            return m_data.SEM.M != 0 && m_data.SEM.E == FeHalfConst::MaxExpVal;
        }

        constexpr bool IsInfinity() const
        {
            return m_data.SEM.M == 0 && m_data.SEM.E == FeHalfConst::MaxExpVal;
        }

        constexpr bool IsDenorm() const
        {
            return m_data.SEM.E == 0;
        }

    private:
        constexpr FeHalf(UInt16 s, UInt16 e, UInt16 m)
            : m_data(0)
        {
            m_data.SEM.S = s;
            m_data.SEM.E = e;
            m_data.SEM.M = m;
        }

        HalfFormat m_data;
    };

    constexpr inline FeHalf operator+(FeHalf l, FeHalf r) noexcept
    {
        return FeHalf(Float32(l) + Float32(r));
    }

    constexpr inline FeHalf operator-(FeHalf l, FeHalf r) noexcept
    {
        return FeHalf(Float32(l) - Float32(r));
    }

    constexpr inline FeHalf operator*(FeHalf l, FeHalf r) noexcept
    {
        return FeHalf(Float32(l) * Float32(r));
    }

    constexpr inline FeHalf operator/(FeHalf l, FeHalf r) noexcept
    {
        return FeHalf(Float32(l) / Float32(r));
    }

    constexpr inline FeHalf& FeHalf::operator+=(FeHalf other) noexcept
    {
        *this = *this + other;
        return *this;
    }

    constexpr inline FeHalf& FeHalf::operator-=(FeHalf other) noexcept
    {
        *this = *this - other;
        return *this;
    }

    constexpr inline FeHalf& FeHalf::operator*=(FeHalf other) noexcept
    {
        *this = *this * other;
        return *this;
    }

    constexpr inline FeHalf& FeHalf::operator/=(FeHalf other) noexcept
    {
        *this = *this / other;
        return *this;
    }
} // namespace FE

using half    = FE::FeHalf;
using float16 = half;

namespace std
{
    constexpr inline half sqrt(half val)
    {
        return sqrtf(val);
    }

    constexpr inline half pow(half x, half y)
    {
        return powf(x, y);
    }

    inline std::ostream& operator<<(std::ostream& stream, half val)
    {
        stream << static_cast<FE::Float32>(val);
        return stream;
    }
} // namespace std
