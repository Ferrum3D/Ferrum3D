#pragma once
#include <FeCore/Modules/Environment.h>

namespace FE::Graphics::Core
{
    enum class AdapterKind : uint32_t
    {
        kNone,
        kIntegrated,
        kDiscrete,
        kVirtual,
        kCPU
    };

    struct AdapterInfo final
    {
        AdapterKind m_kind;
        Env::Name m_name;
    };
} // namespace FE::Graphics::Core
