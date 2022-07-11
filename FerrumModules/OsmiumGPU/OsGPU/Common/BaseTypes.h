#pragma once
#include <FeCore/Base/Base.h>
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
        Int64 X;
        Int64 Y;
        Int64 Z;

        FE_STRUCT_RTTI(Offset, "761BFA99-DC5D-400B-9117-92ED2C1AD3EB");

        inline Offset() noexcept
            : Offset(0, 0, 0)
        {
        }

        inline Offset(Int64 x) noexcept
            : X(x)
            , Y(0)
            , Z(0)
        {
        }

        inline Offset(Int64 x, Int64 y) noexcept
            : X(x)
            , Y(y)
            , Z(0)
        {
        }

        inline Offset(Int64 x, Int64 y, Int64 z) noexcept
            : X(x)
            , Y(y)
            , Z(z)
        {
        }
    };

    struct Size
    {
        UInt64 Width;
        UInt64 Height;
        UInt64 Depth;

        FE_STRUCT_RTTI(Size, "C32E4F84-3144-4D6D-A4D3-00F19E22D708");

        inline Size() noexcept
            : Size(0, 0, 1)
        {
        }

        inline Size(UInt64 w) noexcept
            : Width(w)
            , Height(0)
            , Depth(1)
        {
        }

        inline Size(UInt64 w, UInt64 h) noexcept
            : Width(w)
            , Height(h)
            , Depth(1)
        {
        }

        inline Size(UInt64 w, UInt64 h, UInt64 d) noexcept
            : Width(w)
            , Height(h)
            , Depth(d)
        {
        }
    };
} // namespace FE::Osmium

namespace std
{
    template<>
    struct std::hash<FE::Osmium::Size>
    {
        inline size_t operator()(const FE::Osmium::Size& size) const noexcept
        {
            size_t seed = 0;
            FE::HashCombine(seed, size.Width, size.Height, size.Depth);
            return seed;
        }
    };
} // namespace std
