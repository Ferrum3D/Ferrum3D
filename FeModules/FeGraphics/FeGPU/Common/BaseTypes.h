#pragma once
#include <cstdint>

namespace FE::GPU
{
    struct Size
    {
        uint64_t Width;
        uint64_t Height;
        uint64_t Depth;

        inline Size() noexcept
            : Size(0, 0, 0)
        {
        }

        inline Size(uint64_t w) noexcept
            : Width(w)
            , Height(0)
            , Depth(0)
        {
        }

        inline Size(uint64_t w, uint64_t h) noexcept
            : Width(w)
            , Height(h)
            , Depth(0)
        {
        }

        inline Size(uint64_t w, uint64_t h, uint64_t d) noexcept
            : Width(w)
            , Height(h)
            , Depth(d)
        {
        }
    };
} // namespace FE::GPU
