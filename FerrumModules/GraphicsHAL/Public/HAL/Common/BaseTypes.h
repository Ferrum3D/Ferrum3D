#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/RTTI/RTTI.h>
#include <cstdint>

namespace FE::Graphics::HAL
{
    enum class ResultCode : int32_t
    {
        Success = 0,
        UnknownError = DefaultErrorCode<ResultCode>,
    };


    enum class GraphicsAPI
    {
        None,
        Vulkan
    };


    enum class HardwareQueueKindFlags
    {
        kNone = 0, //!< Invalid or unspecified value.

        kGraphicsBit = 1 << 0, //!< Queue that supports graphics operations.
        kComputeBit = 1 << 1,  //!< Queue that supports compute operations.
        kTransferBit = 1 << 2, //!< Queue that supports copy operations.

        //! @brief Queue for graphics + compute + copy operations.
        kGraphics = kGraphicsBit | kComputeBit | kTransferBit,
        //! @brief Queue for compute + copy operations.
        kCompute = kComputeBit | kTransferBit,
        //! @brief Queue for copy operations.
        kTransfer = kTransferBit,
    };

    FE_ENUM_OPERATORS(HardwareQueueKindFlags);


    struct Offset
    {
        int32_t X = 0;
        int32_t Y = 0;
        int32_t Z = 0;

        inline constexpr Offset() = default;

        inline constexpr explicit Offset(int32_t x) noexcept
            : X(x)
            , Y(0)
            , Z(0)
        {
        }

        inline constexpr Offset(int32_t x, int32_t y) noexcept
            : X(x)
            , Y(y)
            , Z(0)
        {
        }

        inline constexpr Offset(int32_t x, int32_t y, int32_t z) noexcept
            : X(x)
            , Y(y)
            , Z(z)
        {
        }
    };


    struct Size
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t Depth = 1;

        inline constexpr Size() = default;

        inline constexpr explicit Size(uint32_t w) noexcept
            : Width(w)
            , Height(0)
            , Depth(1)
        {
        }

        inline constexpr Size(uint32_t w, uint32_t h) noexcept
            : Width(w)
            , Height(h)
            , Depth(1)
        {
        }

        inline constexpr Size(uint32_t w, uint32_t h, uint32_t d) noexcept
            : Width(w)
            , Height(h)
            , Depth(d)
        {
        }
    };


    inline uint32_t CalculateMipCount(Size size)
    {
        return 1 + Math::FloorLog2(std::max(size.Width, std::max(size.Height, size.Depth)));
    }
} // namespace FE::Graphics::HAL


template<>
struct eastl::hash<FE::Graphics::HAL::Size>
{
    inline size_t operator()(const FE::Graphics::HAL::Size& size) const noexcept
    {
        size_t seed = 0;
        FE::HashCombine(seed, size.Width, size.Height, size.Depth);
        return seed;
    }
};
