#pragma once
#include <FeCore/Memory/Object.h>
#include <FeGPU/Memory/IDeviceMemory.h>
#include <FeGPU/Resource/IResource.h>

namespace FE::GPU
{
    class IDeviceMemory;

    struct BufferDesc
    {
        FE_STRUCT_RTTI(BufferDesc, "2932FBE9-01B0-49C0-BDD5-ED0AD1A29F43");
        UInt64 Size;
    };

    class IBuffer : public IObject
    {
    public:
        FE_CLASS_RTTI(IBuffer, "2249E029-7ABD-4EEE-9D1D-C59570FD27EF");

        virtual ~IBuffer() = default;

        virtual void* Map(UInt64 offset, UInt64 size = static_cast<UInt64>(-1)) = 0;
        virtual void Unmap()                                                    = 0;

        virtual void AllocateMemory(MemoryType type)                                     = 0;
        virtual void BindMemory(const RefCountPtr<IDeviceMemory>& memory, UInt64 offset) = 0;
    };
} // namespace FE::GPU
