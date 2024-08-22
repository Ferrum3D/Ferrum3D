#pragma once
#include <FeCore/Modules/Environment.h>

namespace FE::Graphics::HAL
{
    enum class AdapterKind
    {
        None,
        Integrated,
        Discrete,
        Virtual,
        CPU
    };

    struct AdapterInfo
    {
        AdapterKind Kind;
        Env::Name Name;
    };
} // namespace FE::Graphics::HAL
