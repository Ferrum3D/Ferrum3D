#pragma once
#include <FeCore/Memory/Object.h>
#include <FeGPU/Resource/IResource.h>
#include <FeGPU/Memory/IDeviceMemory.h>

namespace FE::GPU
{
    class IDeviceMemory;

    struct BufferDesc
    {
        UInt64 Size;
    };

    class IBuffer : public IObject
    {
    public:
        virtual ~IBuffer() = default;

        virtual void* Map(UInt64 offset, UInt64 size = static_cast<UInt64>(-1)) = 0;
        virtual void Unmap()                                                          = 0;

        virtual void AllocateMemory(MemoryType type)                                       = 0;
        virtual void BindMemory(const RefCountPtr<IDeviceMemory>& memory, UInt64 offset) = 0;
    };
} // namespace FE::GPU
