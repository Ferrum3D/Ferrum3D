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

        constexpr Viewport(const float minX, const float maxX, const float minY, const float maxY, const float minZ = 0,
                           const float maxZ = 1.0f)
            : minX(minX)
            , minY(minY)
            , minZ(minZ)
            , maxX(maxX)
            , maxY(maxY)
            , maxZ(maxZ)
        {
        }

        [[nodiscard]] constexpr float Width() const
        {
            return maxX - minX;
        }

        [[nodiscard]] constexpr float Height() const
        {
            return maxY - minY;
        }

        [[nodiscard]] constexpr float Depth() const
        {
            return maxZ - minZ;
        }

        [[nodiscard]] constexpr bool IsValid() const
        {
            return minX <= maxX && minY <= maxY && minZ <= maxZ;
        }

        static const Viewport kInvalid;
    };

    inline const Viewport Viewport::kInvalid = { NAN, NAN, NAN, NAN, NAN, NAN };


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

        constexpr Scissor(const int32_t minX, const int32_t maxX, const int32_t minY, const int32_t maxY)
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

        [[nodiscard]] bool IsValid() const
        {
            return minX <= maxX && minY <= maxY;
        }

        static const Scissor kInvalid;
    };

    inline const Scissor Scissor::kInvalid = { INT32_MAX, INT32_MIN, INT32_MAX, INT32_MIN };
} // namespace FE::Graphics::RHI
