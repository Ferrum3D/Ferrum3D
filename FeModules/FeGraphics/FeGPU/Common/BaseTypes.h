#pragma once
#include <FeCore/Base/Base.h>
#include <cstdint>

namespace FE::GPU
{
    struct Size
    {
        UInt64 Width;
        UInt64 Height;
        UInt64 Depth;

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
} // namespace FE::GPU
