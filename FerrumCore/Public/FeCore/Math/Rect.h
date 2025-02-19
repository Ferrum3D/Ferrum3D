#pragma once
#include <FeCore/Math/Vector2.h>

namespace FE
{
    template<class T>
    struct RectBase final
    {
        union
        {
            __m128 m_simdVector;
            T m_values[4];
            struct
            {
                Vector2Base<T> min;
                Vector2Base<T> max;
            };
            struct
            {
                T left, top, right, bottom;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase()
            : m_simdVector(_mm_setzero_ps())
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase(const Vector2Base<T> min, const Vector2Base<T> max)
            : min(min)
            , max(max)
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

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector2Base<T> Size() const
        {
            return max - min;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE T Width() const
        {
            return right - left;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE T Height() const
        {
            return bottom - top;
        }
    };

    using RectF = RectBase<float>;
    using RectInt = RectBase<int32_t>;
    using RectUint = RectBase<uint32_t>;


    namespace Math
    {
        template<class T>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase<T> Intersect(const RectBase<T> lhs, const RectBase<T> rhs)
        {
            RectBase<T> result;

            if ((lhs.left >= rhs.right) || (rhs.left >= lhs.right) || (lhs.top >= rhs.bottom) || (rhs.top >= lhs.bottom))
                return result;

            result.min = Max(lhs.min, rhs.min);
            result.max = Min(lhs.max, rhs.max);
            return result;
        }


        template<class T>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase<T> FE_VECTORCALL Union(const RectBase<T> lhs, const RectBase<T> rhs)
        {
            RectBase<T> result;
            result.min = Min(lhs.min, rhs.min);
            result.max = Max(lhs.max, rhs.max);
            return result;
        }


        template<class T>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase<T> FE_VECTORCALL Offset(const RectBase<T> rect,
                                                                               const Vector2Base<T> offset)
        {
            RectBase<T> result = rect;
            result.min = result.min + offset;
            result.max = result.max + offset;
            return result;
        }


        template<class T>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase<T> FE_VECTORCALL Inflate(const RectBase<T> rect,
                                                                                const Vector2Base<T> offset)
        {
            RectBase<T> result = rect;
            result.max = result.max + offset;
            return result;
        }


        template<class T>
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE T FE_VECTORCALL Area(const RectBase<T> rect)
        {
            const Vector2Base<T> size = rect.Size();
            return size.x * size.y;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EqualEstimate(const RectF lhs, const RectF rhs,
                                                                               const float epsilon = Constants::Epsilon)
        {
            const __m128 kSignMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
            const __m128 distance = _mm_and_ps(_mm_sub_ps(lhs.m_simdVector, rhs.m_simdVector), kSignMask);
            const uint32_t mask = _mm_movemask_ps(_mm_cmpgt_ps(distance, _mm_set1_ps(epsilon)));
            return mask == 0;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EmptyEstimate(const RectF rect,
                                                                               const float epsilon = Constants::Epsilon)
        {
            const __m128 kSignMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
            const __m128 absValue = _mm_and_ps(rect.m_simdVector, kSignMask);
            const uint32_t mask = _mm_movemask_ps(_mm_cmpgt_ps(absValue, _mm_set1_ps(epsilon)));
            return mask == 0;
        }
    } // namespace Math


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator==(const RectBase<T> lhs, const RectBase<T> rhs)
    {
        return lhs.min == rhs.min && lhs.max == rhs.max;
    }


    template<class T>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL operator!=(const RectBase<T> lhs, const RectBase<T> rhs)
    {
        return lhs.min != rhs.min || lhs.max != rhs.max;
    }
} // namespace FE
