#pragma once
#include <Graphics/RHI/DeviceObject.h>

namespace FE::Graphics::RHI
{
    struct MemoryAllocationDesc final
    {
        uint64_t m_size;
        MemoryType m_type;
    };


    struct DeviceMemory : public DeviceObject
    {
        FE_RTTI_Class(DeviceMemory, "52A70884-939E-42DF-B406-26AC86B8DD51");

        ~DeviceMemory() override = default;

        virtual void* Map(size_t offset, size_t size) = 0;
        virtual void Unmap() = 0;
        virtual const MemoryAllocationDesc& GetDesc() = 0;
    };
} // namespace FE::Graphics::RHI
