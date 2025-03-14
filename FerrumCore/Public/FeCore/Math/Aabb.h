#pragma once
#include <FeCore/Math/Rect.h>
#include <FeCore/Math/Vector3.h>

namespace FE
{
    struct Aabb final
    {
        union
        {
            float m_values[8];
            struct
            {
                __m128 m_simdVectorMin;
                __m128 m_simdVectorMax;
            };
            struct
            {
                Vector3F min, max;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Aabb() = default;

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Aabb(const __m128 min, const __m128 max)
            : m_simdVectorMin(min)
            , m_simdVectorMax(max)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Aabb(const Vector3F min, const Vector3F max)
            : min(min)
            , max(max)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Aabb(const RectF rect, const float zMin = 0.0f, const float zMax = 1.0f)
        {
            m_simdVectorMin = _mm_setr_ps(rect.min.x, rect.min.y, zMin, 0.0f);
            m_simdVectorMax = _mm_setr_ps(rect.max.x, rect.max.y, zMax, 0.0f);
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float* FE_VECTORCALL Data()
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE const float* FE_VECTORCALL Data() const
        {
            return m_values;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Vector3F FE_VECTORCALL Size() const
        {
            return max - min;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool IsValid() const
        {
            return Math::CmpGreaterMask(min, max) == 0;
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE RectF FE_VECTORCALL GetRect() const
        {
            return RectF{ Vector2F{ min.x, min.y }, Vector2F{ max.x, max.y } };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Aabb FE_VECTORCALL Zero()
        {
            return Aabb{ _mm_setzero_ps(), _mm_setzero_ps() };
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE static Aabb FE_VECTORCALL Initial()
        {
            return Aabb{ _mm_set1_ps(Constants::kMaxFloat), _mm_set1_ps(-Constants::kMaxFloat) };
        }
    };


    struct PackedAabb final
    {
        union
        {
            float m_values[6];
            struct
            {
                PackedVector3F min, max;
            };
        };

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedAabb() = default;

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedAabb(const PackedVector3F min, const PackedVector3F max)
            : min(min)
            , max(max)
        {
        }

        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedAabb(const Vector3F min, const Vector3F max)
            : min(min)
            , max(max)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE PackedAabb(const Aabb& aabb)
            : min(aabb.min)
            , max(aabb.max)
        {
        }

        explicit FE_FORCE_INLINE FE_NO_SECURITY_COOKIE operator Aabb() const
        {
            return Aabb{ Vector3F{ min }, Vector3F{ max } };
        }
    };

    static_assert(sizeof(PackedAabb) == 2 * sizeof(PackedVector3F));


    namespace Math
    {
        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Aabb FE_VECTORCALL Union(const Aabb& lhs, const Aabb& rhs)
        {
            return Aabb{ Min(lhs.min, rhs.min), Max(lhs.max, rhs.max) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Aabb FE_VECTORCALL Union(const Aabb& lhs, const Vector3F rhs)
        {
            return Aabb{ Min(lhs.min, rhs), Max(lhs.min, rhs) };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Aabb FE_VECTORCALL Offset(const Aabb& lhs, const Vector3F offset)
        {
            return Aabb{ lhs.min + offset, lhs.max + offset };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE Aabb FE_VECTORCALL Inflate(const Aabb& lhs, const Vector3F rhs)
        {
            return Aabb{ lhs.min, lhs.max + rhs };
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL DistanceSquared(const Aabb& lhs, const Vector3F rhs)
        {
            const Vector3F closestPoint = Clamp(rhs, lhs.min, lhs.max);
            return DistanceSquared(closestPoint, rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE float FE_VECTORCALL Distance(const Aabb& lhs, const Vector3F rhs)
        {
            const Vector3F closestPoint = Clamp(rhs, lhs.min, lhs.max);
            return Distance(closestPoint, rhs);
        }


        FE_FORCE_INLINE FE_NO_SECURITY_COOKIE bool FE_VECTORCALL EqualEstimate(const Aabb& lhs, const Aabb& rhs,
                                                                               const float epsilon = Constants::kEpsilon)
        {
            return EqualEstimate(lhs.min, rhs.min, epsilon) && EqualEstimate(lhs.max, rhs.max, epsilon);
        }
    } // namespace Math
} // namespace FE
