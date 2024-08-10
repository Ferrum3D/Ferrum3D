#pragma once
#include <OsGPU/Memory/MemoryType.h>

namespace FE::Osmium
{
    struct MemoryAllocationDesc
    {
        uint64_t Size;
        MemoryType Type;

        FE_RTTI_Base(MemoryAllocationDesc, "45CEEF1A-B382-4F7E-8FA2-69891C98E773");
    };

    class IDeviceMemory : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IDeviceMemory, "52A70884-939E-42DF-B406-26AC86B8DD51");

        ~IDeviceMemory() override = default;

        virtual void* Map(size_t offset, size_t size) = 0;
        virtual void Unmap() = 0;
        virtual const MemoryAllocationDesc& GetDesc() = 0;
    };
} // namespace FE::Osmium
