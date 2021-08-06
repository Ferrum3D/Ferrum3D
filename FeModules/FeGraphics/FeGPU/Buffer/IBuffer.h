#pragma once
#include <FeGPU/Resource/IResource.h>
#include <FeGPU/Memory/IDeviceMemory.h>

namespace FE::GPU
{
    class IDeviceMemory;

    struct BufferDesc
    {
        uint64_t Size;
    };

    class IBuffer
    {
    public:
        virtual ~IBuffer() = default;

        virtual void* Map(uint64_t offset, uint64_t size = static_cast<uint64_t>(-1)) = 0;
        virtual void Unmap()                                                          = 0;

        virtual void AllocateMemory(MemoryType type)                                       = 0;
        virtual void BindMemory(const RefCountPtr<IDeviceMemory>& memory, uint64_t offset) = 0;
    };
} // namespace FE::GPU
