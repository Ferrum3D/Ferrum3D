#pragma once
#include <iostream>
#include <limits>
#include <stdint.h>

namespace FE
{
    namespace FeHalfConst
    {
        inline constexpr float Denorm         = 1.0f / 16384.0f;
        inline constexpr int32_t BitsMantissa = 10;
        inline constexpr int32_t BitsExponent = 5;
        inline constexpr int32_t MaxExpVal    = 0x1f;
        inline constexpr int32_t Bias         = MaxExpVal / 2;
        inline constexpr int32_t MaxExp       = +Bias;
        inline constexpr int32_t MinExp       = -Bias;

        inline constexpr int32_t DoubleBitsMantissa = 52;
        inline constexpr int32_t DoubleBitsExponent = 11;
        inline constexpr int32_t DoubleMaxExpVal    = 0x7ff;
        inline constexpr int32_t DoubleBias         = DoubleMaxExpVal / 2;

        inline constexpr int32_t FloatBitsMantissa = 23;
        inline constexpr int32_t FloatBitsExponent = 8;
        inline constexpr int32_t FloatMaxExpVal    = 0xff;
        inline constexpr int32_t FloatBias         = FloatMaxExpVal / 2;
    } // namespace FeHalfConst

    union HalfFormat
    {
        uint16_t Data;

        struct
        {
            uint16_t M : FeHalfConst::BitsMantissa;
            uint16_t E : FeHalfConst::BitsExponent;
            uint16_t S : 1;
        } SEM; // Sign-Exponent-Mantissa

        constexpr HalfFormat(uint16_t b)
            : Data(b)
        {
        }
    };

    union FloatFormat
    {
        float Data;

        struct
        {
            uint32_t M : FeHalfConst::FloatBitsMantissa;
            uint32_t E : FeHalfConst::FloatBitsExponent;
            uint32_t S : 1;
        } SEM; // Sign-Exponent-Mantissa
    };

    union DoubleFormat
    {
        double Data;

        struct
        {
            uint64_t M : FeHalfConst::DoubleBitsMantissa;
            uint64_t E : FeHalfConst::DoubleBitsExponent;
            uint64_t S : 1;
        } SEM; // Sign-Exponent-Mantissa
    };

    // TODO: Conversion to double/from double

    /**
    * @brief Half-precision floating point.
    */
    struct FeHalf
    {
        constexpr FeHalf() noexcept
            : m_data(0)
        {
        }

        constexpr FeHalf(int iVal) noexcept
            : FeHalf(float(iVal))
        {
        }

        constexpr FeHalf(float fVal) noexcept
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
                int32_t exp = f.SEM.E - 127;

                if (exp < -24)
                { // Zero in half precision
                    m_data.SEM.M = 0;
                    m_data.SEM.E = 0;
                }
                else if (exp < -14)
                { // Denorm
                    m_data.SEM.E = 0;

                    uint32_t exp1 = (uint32_t)(-14 - exp);
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

        constexpr FeHalf(double dVal) noexcept
            : FeHalf(float(dVal))
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

        constexpr FeHalf& operator=(float other) noexcept
        {
            *this = FeHalf(other);
            return *this;
        }

        constexpr FeHalf& operator=(double other) noexcept
        {
            *this = FeHalf(other);
            return *this;
        }

        constexpr operator float() const noexcept
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
                    float mantissa = float(m_data.SEM.M) / 1024.0f;
                    float sign     = (m_data.SEM.S) ? -1.0f : 1.0f;
                    f.Data         = sign * mantissa * FeHalfConst::Denorm;
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

        constexpr operator double() const noexcept
        {
            return double(float(*this));
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

        constexpr bool operator==(float other) const noexcept
        {
            return float(*this) == other;
        }

        constexpr bool operator==(double other) const noexcept
        {
            return double(*this) == other;
        }

        constexpr bool operator==(int other) const noexcept
        {
            return float(*this) == other;
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
        constexpr FeHalf(uint16_t s, uint16_t e, uint16_t m)
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
        return FeHalf(float(l) + float(r));
    }

    constexpr inline FeHalf operator-(FeHalf l, FeHalf r) noexcept
    {
        return FeHalf(float(l) - float(r));
    }

    constexpr inline FeHalf operator*(FeHalf l, FeHalf r) noexcept
    {
        return FeHalf(float(l) * float(r));
    }

    constexpr inline FeHalf operator/(FeHalf l, FeHalf r) noexcept
    {
        return FeHalf(float(l) / float(r));
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

using half = FE::FeHalf;

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
        stream << float(val);
        return stream;
    }
} // namespace std
