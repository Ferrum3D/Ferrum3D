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

    struct Size
    {
        UInt64 Width;
        UInt64 Height;
        UInt64 Depth;

        FE_STRUCT_RTTI(Size, "C32E4F84-3144-4D6D-A4D3-00F19E22D708");

        inline Size() noexcept
            : Size(0, 0, 0)
        {
        }

        inline Size(UInt64 w) noexcept
            : Width(w)
            , Height(0)
            , Depth(0)
        {
        }

        inline Size(UInt64 w, UInt64 h) noexcept
            : Width(w)
            , Height(h)
            , Depth(0)
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
