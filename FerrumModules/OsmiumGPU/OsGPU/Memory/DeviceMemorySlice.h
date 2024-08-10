#pragma once
#include <OsGPU/Memory/IDeviceMemory.h>

namespace FE::Osmium
{
    struct DeviceMemorySlice final
    {
        IDeviceMemory* Memory;
        size_t ByteOffset = 0;
        size_t ByteSize = 0;

        inline DeviceMemorySlice() = default;

        inline explicit DeviceMemorySlice(IDeviceMemory* memory, size_t offset = 0, size_t size = static_cast<size_t>(-1))
        {
            Memory     = memory;
            ByteOffset = offset;
            ByteSize   = std::min(size, memory->GetDesc().Size - offset);
        }

        inline void* Map(size_t offset, size_t size)
        {
            return Memory->Map(ByteOffset + offset, std::min(size, Memory->GetDesc().Size - offset - ByteOffset));
        }

        inline void Unmap()
        {
            Memory->Unmap();
        }
    };
} // namespace FE::Osmium
