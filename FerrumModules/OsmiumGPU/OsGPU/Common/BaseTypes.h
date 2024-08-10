#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/RTTI/RTTI.h>
#include <cstdint>

namespace FE::Osmium
{
    enum class GraphicsAPI
    {
        None,
        Vulkan
    };

    enum class CommandQueueClass
    {
        Graphics,
        Compute,
        Transfer
    };

    struct Offset
    {
        int64_t X;
        int64_t Y;
        int64_t Z;

        FE_RTTI_Base(Offset, "761BFA99-DC5D-400B-9117-92ED2C1AD3EB");

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

        FE_RTTI_Base(Size, "C32E4F84-3144-4D6D-A4D3-00F19E22D708");

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
} // namespace FE::Osmium


template<>
struct eastl::hash<FE::Osmium::Size>
{
    inline size_t operator()(const FE::Osmium::Size& size) const noexcept
    {
        size_t seed = 0;
        FE::HashCombine(seed, size.Width, size.Height, size.Depth);
        return seed;
    }
};
