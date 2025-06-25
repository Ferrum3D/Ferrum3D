#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    template<class T>
    struct Vector2Base final
    {
        union
        {
            T m_values[2];
            struct
            {
                T x, y;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base(ForceInitType)
            : x(0)
            , y(0)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base(T value)
            : x(value)
            , y(value)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base(T x, T y)
            : x(x)
            , y(y)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE T* Data()
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE const T* Data() const
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE T operator[](const uint32_t index) const
        {
            return m_values[index];
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector2Base Zero()
        {
            return Vector2Base{ 0, 0 };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector2Base LoadUnaligned(const T* values)
        {
            return Vector2Base{ values[0], values[1] };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector2Base LoadAligned(const T* values)
        {
            return Vector2Base{ values[0], values[1] };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector2Base AxisX(const T length = 1)
        {
            return Vector2Base{ length, 0 };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector2Base AxisY(const T length = 1)
        {
            return Vector2Base{ 0, length };
        }

        template<class TOther>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE explicit operator Vector2Base<TOther>() const
        {
            return Vector2Base<TOther>{ static_cast<TOther>(x), static_cast<TOther>(y) };
        }
    };

    using Vector2 = Vector2Base<float>;
    using Vector2Int = Vector2Base<int32_t>;
    using Vector2UInt = Vector2Base<uint32_t>;


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base<T> operator+(Vector2Base<T> lhs, Vector2Base<T> rhs)
    {
        return Vector2Base<T>{ lhs.x + rhs.x, lhs.y + rhs.y };
    }


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base<T> operator-(Vector2Base<T> lhs, Vector2Base<T> rhs)
    {
        return Vector2Base<T>{ lhs.x - rhs.x, lhs.y - rhs.y };
    }


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base<T> operator-(Vector2Base<T> vec)
    {
        return Vector2Base<T>{ -vec.x, -vec.y };
    }


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base<T> operator*(Vector2Base<T> lhs, Vector2Base<T> rhs)
    {
        return Vector2Base<T>{ lhs.x * rhs.x, lhs.y * rhs.y };
    }


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base<T> operator/(Vector2Base<T> lhs, Vector2Base<T> rhs)
    {
        return Vector2Base<T>{ lhs.x / rhs.x, lhs.y / rhs.y };
    }


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base<T> operator*(Vector2Base<T> lhs, T rhs)
    {
        return Vector2Base<T>{ lhs.x * rhs, lhs.y * rhs };
    }


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base<T> operator/(Vector2Base<T> lhs, T rhs)
    {
        return Vector2Base<T>{ lhs.x / rhs, lhs.y / rhs };
    }


    namespace Math
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Dot(const Vector2Base<float> lhs, const Vector2Base<float> rhs)
        {
            return lhs.x * rhs.x + lhs.y * rhs.y;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2 Abs(const Vector2 vec)
        {
            return Vector2{ Abs(vec.x), Abs(vec.y) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Int Abs(const Vector2Int vec)
        {
            return Vector2Int{ Abs(vec.x), Abs(vec.y) };
        }


        template<class T>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base<T> Min(Vector2Base<T> lhs, Vector2Base<T> rhs)
        {
            return Vector2Base<T>{ Min(lhs.x, rhs.x), Min(lhs.y, rhs.y) };
        }


        template<class T>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base<T> Max(Vector2Base<T> lhs, Vector2Base<T> rhs)
        {
            return Vector2Base<T>{ Max(lhs.x, rhs.x), Max(lhs.y, rhs.y) };
        }


        template<class T>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base<T> Clamp(Vector2Base<T> vec, Vector2Base<T> min, Vector2Base<T> max)
        {
            return Vector2Base<T>{ Clamp(vec.x, min.x, max.x), Clamp(vec.y, min.y, max.y) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2 Saturate(const Vector2 vec)
        {
            return Vector2{ Saturate(vec.x), Saturate(vec.y) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2 Floor(const Vector2 vec)
        {
            return Vector2{ Floor(vec.x), Floor(vec.y) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2 Ceil(const Vector2 vec)
        {
            return Vector2{ Ceil(vec.x), Ceil(vec.y) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2 Round(const Vector2 vec)
        {
            return Vector2{ Round(vec.x), Round(vec.y) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2 Reciprocal(const Vector2 vec)
        {
            return Vector2{ 1.0f / vec.x, 1.0f / vec.y };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2 ReciprocalEstimate(const Vector2 vec)
        {
            return Vector2{ 1.0f / vec.x, 1.0f / vec.y };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2 Sqrt(const Vector2 vec)
        {
            return Vector2{ Sqrt(vec.x), Sqrt(vec.y) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2 ReciprocalSqrt(const Vector2 vec)
        {
            return Vector2{ 1.0f / Sqrt(vec.x), 1.0f / Sqrt(vec.y) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2 ReciprocalSqrtEstimate(const Vector2 vec)
        {
            return Vector2{ 1.0f / Sqrt(vec.x), 1.0f / Sqrt(vec.y) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float LengthSquared(const Vector2 vec)
        {
            return Dot(vec, vec);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Length(const Vector2 vec)
        {
            return Sqrt(LengthSquared(vec));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2 Normalize(const Vector2 vec)
        {
            return vec / Length(vec);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float DistanceSquared(const Vector2 lhs, const Vector2 rhs)
        {
            return LengthSquared(lhs - rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Distance(const Vector2 lhs, const Vector2 rhs)
        {
            return Length(lhs - rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool EqualEstimate(const Vector2 lhs, const Vector2 rhs,
                                                                 const float epsilon = Constants::kEpsilon)
        {
            return EqualEstimate(lhs.x, rhs.x, epsilon) && EqualEstimate(lhs.y, rhs.y, epsilon);
        }
    } // namespace Math


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool operator==(Vector2Base<T> lhs, Vector2Base<T> rhs)
    {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool operator!=(Vector2Base<T> lhs, Vector2Base<T> rhs)
    {
        return lhs.x != rhs.x || lhs.y != rhs.y;
    }
} // namespace FE
