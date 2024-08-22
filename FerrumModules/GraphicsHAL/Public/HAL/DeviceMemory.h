#pragma once
#include <HAL/DeviceObject.h>
#include <HAL/MemoryType.h>

namespace FE::Graphics::HAL
{
    struct MemoryAllocationDesc
    {
        uint64_t Size;
        MemoryType Type;
    };


    class DeviceMemory : public DeviceObject
    {
    public:
        FE_RTTI_Class(DeviceMemory, "52A70884-939E-42DF-B406-26AC86B8DD51");

        ~DeviceMemory() override = default;

        virtual void* Map(size_t offset, size_t size) = 0;
        virtual void Unmap() = 0;
        virtual const MemoryAllocationDesc& GetDesc() = 0;
    };
} // namespace FE::Graphics::HAL
