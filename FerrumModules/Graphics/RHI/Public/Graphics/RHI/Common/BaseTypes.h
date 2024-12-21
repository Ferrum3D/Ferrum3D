#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/RTTI/RTTI.h>
#include <cstdint>

namespace FE::Graphics::RHI
{
    enum class ResultCode : int32_t
    {
        kSuccess = 0,
        kUnknownError = kDefaultErrorCode<ResultCode>,
    };


    enum class GraphicsAPI : uint32_t
    {
        kNone,
        kVulkan
    };


    enum class HardwareQueueKindFlags : uint32_t
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


    enum class BindFlags : uint32_t
    {
        kNone = 0,
        kVertexBuffer = 1 << 0,
        kIndexBuffer = 1 << 1,
        kConstantBuffer = 1 << 2,
        kShaderResource = 1 << 3,
        kStreamOutput = 1 << 4,
        kRenderTarget = 1 << 5,
        kDepthStencil = 1 << 6,
        kUnorderedAccess = 1 << 7,
        kIndirectDrawArgs = 1 << 8,
        kInputAttachment = 1 << 9
    };

    FE_ENUM_OPERATORS(BindFlags);


    enum class MemoryType : uint32_t
    {
        kDeviceLocal,
        kHostVisible,
    };


    struct Offset final
    {
        int32_t x = 0;
        int32_t y = 0;
        int32_t z = 0;

        constexpr Offset() = default;

        constexpr explicit Offset(int32_t x)
            : x(x)
            , y(0)
            , z(0)
        {
        }

        constexpr Offset(int32_t x, int32_t y)
            : x(x)
            , y(y)
            , z(0)
        {
        }

        constexpr Offset(int32_t x, int32_t y, int32_t z)
            : x(x)
            , y(y)
            , z(z)
        {
        }
    };


    struct Size final
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 1;

        constexpr Size() = default;

        constexpr explicit Size(uint32_t w)
            : width(w)
            , height(0)
            , depth(1)
        {
        }

        constexpr Size(uint32_t w, uint32_t h)
            : width(w)
            , height(h)
            , depth(1)
        {
        }

        constexpr Size(uint32_t w, uint32_t h, uint32_t d)
            : width(w)
            , height(h)
            , depth(d)
        {
        }
    };


    inline uint32_t CalculateMipCount(Size size)
    {
        return 1 + Math::FloorLog2(std::max(size.width, std::max(size.height, size.depth)));
    }
} // namespace FE::Graphics::RHI


template<>
struct eastl::hash<FE::Graphics::RHI::Size>
{
    inline size_t operator()(const FE::Graphics::RHI::Size& size) const noexcept
    {
        size_t seed = 0;
        FE::HashCombine(seed, size.width, size.height, size.depth);
        return seed;
    }
};
