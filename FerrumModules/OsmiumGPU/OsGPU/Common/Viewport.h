#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Osmium
{
    struct Viewport
    {
        float MinX;
        float MinY;
        float MinZ;
        float MaxX;
        float MaxY;
        float MaxZ;

        Viewport() = default;

        inline Viewport(float minX, float maxX, float minY, float maxY, float minZ = 0, float maxZ = 1.0f)
            : MinX(minX)
            , MinY(minY)
            , MinZ(minZ)
            , MaxX(maxX)
            , MaxY(maxY)
            , MaxZ(maxZ)
        {
        }

        [[nodiscard]] inline float Width() const noexcept
        {
            return MaxX - MinX;
        }

        [[nodiscard]] inline float Height() const noexcept
        {
            return MaxY - MinY;
        }

        [[nodiscard]] inline float Depth() const noexcept
        {
            return MaxZ - MinZ;
        }
    };

    struct Scissor
    {
        int32_t MinX;
        int32_t MinY;
        int32_t MaxX;
        int32_t MaxY;

        Scissor() = default;

        inline explicit Scissor(const Viewport& viewport)
            : MinX(static_cast<int32_t>(viewport.MinX))
            , MinY(static_cast<int32_t>(viewport.MinY))
            , MaxX(static_cast<int32_t>(viewport.MaxX))
            , MaxY(static_cast<int32_t>(viewport.MaxY))
        {
        }

        inline Scissor(int32_t minX, int32_t maxX, int32_t minY, int32_t maxY)
            : MinX(minX)
            , MinY(minY)
            , MaxX(maxX)
            , MaxY(maxY)
        {
        }

        [[nodiscard]] inline int32_t Width() const noexcept
        {
            return MaxX - MinX;
        }

        [[nodiscard]] inline int32_t Height() const noexcept
        {
            return MaxY - MinY;
        }
    };
} // namespace FE::Osmium
