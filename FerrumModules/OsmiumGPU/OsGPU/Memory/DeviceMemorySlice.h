#pragma once
#include <FeCore/Memory/SharedPtr.h>
#include <OsGPU/Memory/IDeviceMemory.h>

namespace FE::Osmium
{
    struct DeviceMemorySlice final
    {
        IDeviceMemory* Memory;
        USize ByteOffset = 0;
        USize ByteSize = 0;

        inline DeviceMemorySlice() = default;

        inline explicit DeviceMemorySlice(IDeviceMemory* memory, USize offset = 0, USize size = static_cast<USize>(-1))
        {
            Memory     = memory;
            ByteOffset = offset;
            ByteSize   = std::min(size, memory->GetDesc().Size - offset);
        }

        inline void* Map(USize offset, USize size)
        {
            return Memory->Map(ByteOffset + offset, std::min(size, Memory->GetDesc().Size - offset - ByteOffset));
        }

        inline void Unmap()
        {
            Memory->Unmap();
        }
    };
} // namespace FE::Osmium
