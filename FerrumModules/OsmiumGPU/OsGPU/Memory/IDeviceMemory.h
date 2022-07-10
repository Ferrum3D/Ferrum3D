#pragma once
#include <FeCore/Memory/Object.h>
#include <OsGPU/Memory/MemoryType.h>

namespace FE::Osmium
{
    struct MemoryAllocationDesc
    {
        UInt64 Size;
        MemoryType Type;

        FE_STRUCT_RTTI(MemoryAllocationDesc, "45CEEF1A-B382-4F7E-8FA2-69891C98E773");
    };

    class IDeviceMemory : public IObject
    {
    public:
        FE_CLASS_RTTI(IDeviceMemory, "52A70884-939E-42DF-B406-26AC86B8DD51");

        ~IDeviceMemory() override = default;

        virtual void* Map(USize offset, USize size) = 0;
        virtual void Unmap() = 0;
        virtual const MemoryAllocationDesc& GetDesc() = 0;
    };
} // namespace FE::Osmium
