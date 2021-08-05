#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::GPU
{
    class IDeviceMemory;

    class IResource
    {
    public:
        virtual ~IResource() = default;

        virtual void* Map()  = 0;
        virtual void Unmap() = 0;

        virtual void BindMemory(const RefCountPtr<IDeviceMemory>& memory, uint64_t offset) = 0;
    };
} // namespace FE::GPU
