#pragma once
#include "FeHalf.h"
#include <cstdint>
#include <iostream>

namespace FE
{
    template<class T>
    struct FeVector4
    {
    public:
        union
        {
            struct
            {
                T X, Y, Z, W;
            };
            T Data[4];
        };

        inline constexpr FeVector4()
        {
            X = Y = Z = 0;
        }

        inline constexpr FeVector4(const T xyz)
        {
            X = Y = Z = W = xyz;
        }

        inline constexpr FeVector4(const T x, const T y, const T z, const T w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        inline constexpr T LengthSquared()
        {
            return X * X + Y * Y + Z * Z + W * W;
        }

        inline constexpr T Length()
        {
            return std::sqrt(LengthSquared());
        }

        inline constexpr FeVector4<T>& operator+=(const FeVector4<T>& other)
        {
            X += other.X;
            Y += other.Y;
            Z += other.Z;
            W += other.W;
            return *this;
        }

        inline constexpr FeVector4<T>& operator-=(const FeVector4<T>& other)
        {
            X -= other.X;
            Y -= other.Y;
            Z -= other.Z;
            W -= other.W;
            return *this;
        }

        inline constexpr FeVector4<T>& operator*=(const T factor)
        {
            X *= factor;
            Y *= factor;
            Z *= factor;
            W *= factor;
            return *this;
        }

        inline constexpr FeVector4<T>& operator/=(const T divisor)
        {
            X /= divisor;
            Y /= divisor;
            Z /= divisor;
            W /= divisor;
            return *this;
        }

        inline constexpr FeVector4<T> operator+(const FeVector4<T>& other)
        {
            FeVector4<T> result{ *this };
            result += other;
            return result;
        }

        inline constexpr FeVector4<T> operator-(const FeVector4<T>& other)
        {
            FeVector4<T> result{ *this };
            result -= other;
            return result;
        }

        inline constexpr FeVector4<T> operator*(const T factor)
        {
            FeVector4<T> result{ *this };
            result *= factor;
            return result;
        }

        inline constexpr FeVector4<T> operator/(const T divisor)
        {
            FeVector4<T> result{ *this };
            result /= divisor;
            return result;
        }

        inline constexpr bool operator==(const FeVector4<float>& other)
        {
            return X == other.X && Y == other.Y && Z == other.Z && W == other.W;
        }

        inline constexpr bool operator!=(const FeVector4<float>& other)
        {
            return X != other.X || Y != other.Y || Z != other.Z;
        }
    };

    template<class T>
    inline constexpr T DotProd(const FeVector4<T>& left, const FeVector4<T>& right)
    {
        return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
    }

    template<class T>
    inline std::ostream& operator<<(std::ostream& stream, const FeVector4<T>& vector)
    {
        stream << "{ " << vector.X << "; " << vector.Y << "; " << vector.Z << "; " << vector.W << " }";
        return stream;
    }

    using float4  = FeVector4<float>;
    using double4 = FeVector4<double>;
    using int4    = FeVector4<int32_t>;
    using uint4   = FeVector4<uint32_t>;
    using half4   = FeVector4<half>;
} // namespace FE
