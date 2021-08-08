#pragma once
#include <FeCore/Memory/Object.h>
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
        UInt64 Size;
        MemoryType Type;
    };

    class IDeviceMemory : public IObject
    {
    public:
        virtual ~IDeviceMemory() = default;

        virtual const MemoryAllocationDesc& GetDesc() = 0;
    };
} // namespace FE::GPU
