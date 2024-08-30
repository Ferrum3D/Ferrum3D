#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Memory/Memory.h>

namespace FE::Graphics::HAL
{
    enum class MemoryType
    {
        kDeviceLocal,
        kHostVisible
    };
} // namespace FE::Graphics::HAL
