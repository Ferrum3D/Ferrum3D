#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Graphics::HAL
{
    struct Viewport
    {
        float MinX = 0.0f;
        float MinY = 0.0f;
        float MinZ = 0.0f;
        float MaxX = 0.0f;
        float MaxY = 0.0f;
        float MaxZ = 0.0f;

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
        int32_t MinX = 0;
        int32_t MinY = 0;
        int32_t MaxX = 0;
        int32_t MaxY = 0;

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
} // namespace FE::Graphics::HAL
