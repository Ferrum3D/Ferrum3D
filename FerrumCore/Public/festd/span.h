#pragma once
#include <EASTL/span.h>

namespace FE::festd
{
    inline constexpr size_t dynamic_extent = static_cast<size_t>(-1);
    using eastl::span;
} // namespace FE::festd
