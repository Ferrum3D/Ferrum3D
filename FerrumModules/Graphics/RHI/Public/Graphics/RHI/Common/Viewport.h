#pragma once
#include <FeCore/Base/Base.h>

namespace FE::Graphics::RHI
{
    struct Viewport final
    {
        float minX = 0.0f;
        float minY = 0.0f;
        float minZ = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        float maxZ = 0.0f;

        Viewport() = default;

        Viewport(float minX, float maxX, float minY, float maxY, float minZ = 0, float maxZ = 1.0f)
            : minX(minX)
            , minY(minY)
            , minZ(minZ)
            , maxX(maxX)
            , maxY(maxY)
            , maxZ(maxZ)
        {
        }

        [[nodiscard]] float Width() const
        {
            return maxX - minX;
        }

        [[nodiscard]] float Height() const
        {
            return maxY - minY;
        }

        [[nodiscard]] float Depth() const
        {
            return maxZ - minZ;
        }
    };

    struct Scissor final
    {
        int32_t minX = 0;
        int32_t minY = 0;
        int32_t maxX = 0;
        int32_t maxY = 0;

        Scissor() = default;

        explicit Scissor(const Viewport& viewport)
            : minX(static_cast<int32_t>(viewport.minX))
            , minY(static_cast<int32_t>(viewport.minY))
            , maxX(static_cast<int32_t>(viewport.maxX))
            , maxY(static_cast<int32_t>(viewport.maxY))
        {
        }

        Scissor(int32_t minX, int32_t maxX, int32_t minY, int32_t maxY)
            : minX(minX)
            , minY(minY)
            , maxX(maxX)
            , maxY(maxY)
        {
        }

        [[nodiscard]] int32_t Width() const
        {
            return maxX - minX;
        }

        [[nodiscard]] int32_t Height() const
        {
            return maxY - minY;
        }
    };
} // namespace FE::Graphics::RHI
