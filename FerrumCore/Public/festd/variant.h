#pragma once
#include <EASTL/variant.h>

namespace FE::festd
{
    using eastl::get;
    using eastl::get_if;
    using eastl::holds_alternative;
    using eastl::variant;
    using eastl::variant_alternative_t;
    using eastl::visit;
} // namespace FE::festd
