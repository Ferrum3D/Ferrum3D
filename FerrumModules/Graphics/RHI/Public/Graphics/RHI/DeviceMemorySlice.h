#pragma once
#include <Graphics/RHI/DeviceMemory.h>

namespace FE::Graphics::RHI
{
    struct DeviceMemorySlice final
    {
        DeviceMemory* m_memory = nullptr;
        size_t m_byteOffset = 0;
        size_t m_byteSize = 0;

        DeviceMemorySlice() = default;

        explicit DeviceMemorySlice(DeviceMemory* memory, size_t offset = 0, size_t size = static_cast<size_t>(-1))
        {
            m_memory = memory;
            m_byteOffset = offset;
            m_byteSize = std::min(size, memory->GetDesc().m_size - offset);
        }

        void* Map(size_t offset, size_t size)
        {
            return m_memory->Map(m_byteOffset + offset, std::min(size, m_memory->GetDesc().m_size - offset - m_byteOffset));
        }

        void Unmap()
        {
            m_memory->Unmap();
        }
    };
} // namespace FE::Graphics::RHI
