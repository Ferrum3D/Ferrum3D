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

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector2Base AxisX(T length = 1.0f)
        {
            return Vector2Base{ length, 0 };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Vector2Base AxisY(T length = 1.0f)
        {
            return Vector2Base{ 0, length };
        }
    };

    using Vector2F = Vector2Base<float>;
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
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Dot(Vector2Base<float> lhs, Vector2Base<float> rhs)
        {
            return lhs.x * rhs.x + lhs.y * rhs.y;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2F Abs(Vector2F vec)
        {
            return Vector2F{ Abs(vec.x), Abs(vec.y) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Int Abs(Vector2Int vec)
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


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2F Saturate(Vector2F vec)
        {
            return Vector2F{ Saturate(vec.x), Saturate(vec.y) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float LengthSquared(Vector2F vec)
        {
            return Dot(vec, vec);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Length(Vector2F vec)
        {
            return Sqrt(LengthSquared(vec));
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2F Normalize(Vector2F vec)
        {
            return vec / Length(vec);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float DistanceSquared(Vector2F lhs, Vector2F rhs)
        {
            return LengthSquared(lhs - rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float Distance(Vector2F lhs, Vector2F rhs)
        {
            return Length(lhs - rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool EqualEstimate(Vector2F lhs, Vector2F rhs, float epsilon = Constants::kEpsilon)
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
