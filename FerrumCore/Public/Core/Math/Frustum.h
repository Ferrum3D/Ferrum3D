#pragma once
#include <Core/Math/Aabb.h>
#include <Core/Math/Matrix4x4.h>

namespace FE
{
    //! Inward planes for row-vector clip transforms with depth in [0, w], including reversed/infinite Z.
    struct Frustum final
    {
        explicit Frustum(const Matrix4x4& viewProjection)
        {
            const Vector4 x(viewProjection.m_00, viewProjection.m_10, viewProjection.m_20, viewProjection.m_30);
            const Vector4 y(viewProjection.m_01, viewProjection.m_11, viewProjection.m_21, viewProjection.m_31);
            const Vector4 z(viewProjection.m_02, viewProjection.m_12, viewProjection.m_22, viewProjection.m_32);
            const Vector4 w(viewProjection.m_03, viewProjection.m_13, viewProjection.m_23, viewProjection.m_33);

            m_planes[0] = w + x;
            m_planes[1] = w - x;
            m_planes[2] = w + y;
            m_planes[3] = w - y;
            m_planes[4] = z;
            m_planes[5] = w - z;

            for (Vector4& plane : m_planes)
            {
                // Scale the tolerance without normalizing: zero-normal infinite planes remain valid.
                plane.w += 1e-4f * Math::Sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
            }
        }

        [[nodiscard]] bool Overlaps(const Aabb& bounds) const
        {
            for (const Vector4& plane : m_planes)
            {
                const float x = plane.x >= 0.0f ? bounds.max.x : bounds.min.x;
                const float y = plane.y >= 0.0f ? bounds.max.y : bounds.min.y;
                const float z = plane.z >= 0.0f ? bounds.max.z : bounds.min.z;
                if (plane.x * x + plane.y * y + plane.z * z + plane.w < 0.0f)
                    return false;
            }

            return true;
        }

        Vector4 m_planes[6];
    };
} // namespace FE
