#pragma once
#include <cstdint>

namespace FE::GPU
{
    enum class MemoryType
    {
        DeviceLocal,
        HostVisible
    };

    struct MemoryAllocationDesc
    {
        uint64_t Size;
        MemoryType Type;
    };

    class IDeviceMemory
    {
    public:
        virtual ~IDeviceMemory() = default;

        virtual const MemoryAllocationDesc& GetDesc() = 0;
    };
} // namespace FE::GPU
