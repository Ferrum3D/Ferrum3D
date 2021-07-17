#pragma once
#include "FeHalf.h"
#include <cstdint>
#include <iostream>

namespace FE
{
    template<class T>
    struct FeVector3
    {
    public:
        union
        {
            struct
            {
                T X, Y, Z;
            };
            T Data[3];
        };

        inline constexpr FeVector3()
        {
            X = Y = Z = 0;
        }

        inline constexpr FeVector3(const T xyz)
        {
            X = Y = Z = xyz;
        }

        inline constexpr FeVector3(const T x, const T y, const T z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        inline constexpr T LengthSquared()
        {
            return X * X + Y * Y + Z * Z;
        }

        inline constexpr T Length()
        {
            return std::sqrt(LengthSquared());
        }

        inline constexpr FeVector3<T>& operator+=(const FeVector3<T>& other)
        {
            X += other.X;
            Y += other.Y;
            Z += other.Z;
            return *this;
        }

        inline constexpr FeVector3<T>& operator-=(const FeVector3<T>& other)
        {
            X -= other.X;
            Y -= other.Y;
            Z -= other.Z;
            return *this;
        }

        inline constexpr FeVector3<T>& operator*=(const T factor)
        {
            X *= factor;
            Y *= factor;
            Z *= factor;
            return *this;
        }

        inline constexpr FeVector3<T>& operator/=(const T divisor)
        {
            X /= divisor;
            Y /= divisor;
            Z /= divisor;
            return *this;
        }

        inline constexpr FeVector3<T> operator+(const FeVector3<T>& other)
        {
            FeVector3<T> result{ *this };
            result += other;
            return result;
        }

        inline constexpr FeVector3<T> operator-(const FeVector3<T>& other)
        {
            FeVector3<T> result{ *this };
            result -= other;
            return result;
        }

        inline constexpr FeVector3<T> operator*(const T factor)
        {
            FeVector3<T> result{ *this };
            result *= factor;
            return result;
        }

        inline constexpr FeVector3<T> operator/(const T divisor)
        {
            FeVector3<T> result{ *this };
            result /= divisor;
            return result;
        }

        inline constexpr bool operator==(const FeVector3<float>& other)
        {
            return X == other.X && Y == other.Y && Z == other.Z;
        }

        inline constexpr bool operator!=(const FeVector3<float>& other)
        {
            return X != other.X || Y != other.Y || Z != other.Z;
        }

        inline constexpr void CrossProd(const FeVector3<T>& other)
        {
            //   _  _  _
            // | i  j  k  |
            // | x1 y1 z1 | = { y1*z2 - y2*z1; -(x1*z2 - x2*z1); x1*y2 - x2*y1 } = { y1*z2 - y2*z1; x2*z1 - x1*z2; x1*y2 - x2*y1 }
            // | x2 y2 z2 |
            T x = Y * other.Z - other.Y * Z;
            T y = Z * other.X - other.Z * X;
            T z = X * other.Y - other.X * Y;
            X   = x;
            Y   = y;
            Z   = z;
        }
    };

    template<class T>
    inline constexpr T DotProd(const FeVector3<T>& left, const FeVector3<T>& right)
    {
        return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
    }

    template<class T>
    inline constexpr FeVector3<T> CrossProd(const FeVector3<T>& left, const FeVector3<T>& right)
    {
        FeVector3<T> result{ left };
        result.CrossProd(right);
        return result;
    }

    template<class T>
    inline std::ostream& operator<<(std::ostream& stream, const FeVector3<T>& vector)
    {
        stream << "{ " << vector.X << "; " << vector.Y << "; " << vector.Z << " }";
        return stream;
    }

    using float3  = FeVector3<float>;
    using double3 = FeVector3<double>;
    using int3    = FeVector3<int32_t>;
    using uint3   = FeVector3<uint32_t>;
    using half3   = FeVector3<half>;
} // namespace FE
