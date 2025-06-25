#pragma once
#include <FeCore/Math/Vector2.h>

namespace FE
{
    template<class T>
    struct RectBase final
    {
        using SimdVecType = std::conditional_t<
            std::is_same_v<T, float>, __m128,
            std::conditional_t<std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>, __m128i, EmptyStruct>>;

        static constexpr bool kIsSimd = !std::is_same_v<T, EmptyStruct>;

        union
        {
            SimdVecType m_simdVector;
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

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase() = default;

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase(ForceInitType)
        {
            if constexpr (std::is_same_v<T, float>)
            {
                m_simdVector = _mm_setzero_ps();
            }
            else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t>)
            {
                m_simdVector = _mm_setzero_si128();
            }
            else
            {
                left = 0;
                top = 0;
                right = 0;
                bottom = 0;
            }
        }

        FE_FORCE_INLINE RectBase(const RectBase&) = default;
        FE_FORCE_INLINE RectBase(RectBase&&) = default;
        FE_FORCE_INLINE RectBase& operator=(const RectBase&) = default;
        FE_FORCE_INLINE RectBase& operator=(RectBase&&) = default;
        FE_FORCE_INLINE ~RectBase() = default;

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase(const Vector2Base<T> min, const Vector2Base<T> max)
            : min(min)
            , max(max)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase(const T left, const T top, const T right, const T bottom)
            : left(left)
            , top(top)
            , right(right)
            , bottom(bottom)
        {
        }

        template<class TOther>
        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase(const RectBase<TOther> other)
            : left(static_cast<T>(other.left))
            , top(static_cast<T>(other.top))
            , right(static_cast<T>(other.right))
            , bottom(static_cast<T>(other.bottom))
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static RectBase FE_VECTORCALL FromPosAndSize(const Vector2Base<T> pos,
                                                                                           const Vector2Base<T> size)
        {
            return RectBase{ pos, pos + size };
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

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool IsValid() const
        {
            return left <= right && top <= bottom;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static RectBase FE_VECTORCALL Zero()
        {
            return RectBase{ 0, 0, 0, 0 };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static RectBase FE_VECTORCALL Initial()
        {
            const Vector2Base<T> min{ Constants::kMaxValue<T> };
            const Vector2Base<T> max{ Constants::kMinValue<T> };
            return RectBase{ min, max };
        }
    };

    using RectF = RectBase<float>;
    using RectInt = RectBase<int32_t>;
    using RectUInt = RectBase<uint32_t>;


    template<>
    template<>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase<float>::RectBase(const RectBase<int32_t> other)
        : m_simdVector(_mm_cvtepi32_ps(other.m_simdVector))
    {
    }


    template<>
    template<>
    FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectBase<int32_t>::RectBase(const RectBase<float> other)
        : m_simdVector(_mm_cvtps_epi32(other.m_simdVector))
    {
    }


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
                                                                               const float epsilon = Constants::kEpsilon)
        {
            const __m128 kSignMask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
            const __m128 distance = _mm_and_ps(_mm_sub_ps(lhs.m_simdVector, rhs.m_simdVector), kSignMask);
            const uint32_t mask = _mm_movemask_ps(_mm_cmpgt_ps(distance, _mm_set1_ps(epsilon)));
            return mask == 0;
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EmptyEstimate(const RectF rect,
                                                                               const float epsilon = Constants::kEpsilon)
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
