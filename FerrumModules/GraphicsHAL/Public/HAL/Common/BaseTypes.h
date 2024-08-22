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
        None = 0, //!< Invalid or unspecified value.

        GraphicsBit = 1 << 0, //!< Queue that supports graphics operations.
        ComputeBit = 1 << 1,  //!< Queue that supports compute operations.
        TransferBit = 1 << 2, //!< Queue that supports copy operations.

        //! \brief Queue for graphics + compute + copy operations.
        Graphics = GraphicsBit | ComputeBit | TransferBit,
        //! \brief Queue for compute + copy operations.
        Compute = ComputeBit | TransferBit,
        //! \brief Queue for copy operations.
        Transfer = TransferBit,
    };

    FE_ENUM_OPERATORS(HardwareQueueKindFlags);


    enum class FenceState
    {
        Signaled,
        Reset
    };


    struct Offset
    {
        int64_t X;
        int64_t Y;
        int64_t Z;

        inline Offset() noexcept
            : Offset(0, 0, 0)
        {
        }

        inline Offset(int64_t x) noexcept
            : X(x)
            , Y(0)
            , Z(0)
        {
        }

        inline Offset(int64_t x, int64_t y) noexcept
            : X(x)
            , Y(y)
            , Z(0)
        {
        }

        inline Offset(int64_t x, int64_t y, int64_t z) noexcept
            : X(x)
            , Y(y)
            , Z(z)
        {
        }
    };


    struct Size
    {
        uint64_t Width;
        uint64_t Height;
        uint64_t Depth;

        inline Size() noexcept
            : Size(0, 0, 1)
        {
        }

        inline Size(uint64_t w) noexcept
            : Width(w)
            , Height(0)
            , Depth(1)
        {
        }

        inline Size(uint64_t w, uint64_t h) noexcept
            : Width(w)
            , Height(h)
            , Depth(1)
        {
        }

        inline Size(uint64_t w, uint64_t h, uint64_t d) noexcept
            : Width(w)
            , Height(h)
            , Depth(d)
        {
        }
    };
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
