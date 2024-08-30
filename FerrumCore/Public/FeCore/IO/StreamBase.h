#pragma once
#include <FeCore/IO/IStream.h>
#include <FeCore/Logging/Trace.h>

namespace FE::IO
{
    struct StreamBase : public IStream
    {
        FE_RTTI_Class(StreamBase, "2F74FF8D-4D81-44BE-962A-9D30669E03C8");

        ~StreamBase() override = default;

        inline bool WriteAllowed() const override
        {
            return IsWriteAllowed(GetOpenMode());
        }

        inline bool ReadAllowed() const override
        {
            return IsReadAllowed(GetOpenMode());
        }

        inline size_t WriteFromStream(IStream* stream, size_t size) override
        {
            FE_AssertMsg(stream, "Stream was nullptr");
            FE_AssertMsg(stream->ReadAllowed(), "Source stream was write-only");
            FE_AssertMsg(WriteAllowed(), "Destination stream was read-only");
            FE_AssertMsg(stream != this, "Destination and source streams are the same");

            std::byte tempBuffer[512];
            size_t result = 0;

            for (size_t offset = 0; offset < size; offset += sizeof(tempBuffer))
            {
                const size_t remaining = size - offset;
                const size_t currentSize = std::min(remaining, sizeof(tempBuffer));
                stream->ReadToBuffer({ tempBuffer, static_cast<uint32_t>(currentSize) });
                result += WriteFromBuffer({ tempBuffer, static_cast<uint32_t>(currentSize) });
            }

            return result;
        }

        inline void FlushWrites() override {}

        inline FileStats GetStats() const override
        {
            FE_AssertMsg(false, "Not supported");
            return {};
        }
    };


    class BufferedStream : public StreamBase
    {
    protected:
        std::pmr::memory_resource* m_pBufferAllocator = nullptr;
        std::byte* m_pBuffer = nullptr;
        uint32_t m_BufferCapacity = 1024;
        uint32_t m_BufferPosition = 0;

        inline BufferedStream(std::pmr::memory_resource* pBufferAllocator)
        {
            SetBufferAllocator(pBufferAllocator);
        }

        virtual size_t WriteImpl(festd::span<const std::byte> buffer) = 0;

    public:
        inline ~BufferedStream()
        {
            if (m_pBuffer)
            {
                m_pBufferAllocator->deallocate(m_pBuffer, m_BufferCapacity, Memory::kDefaultAlignment);
                m_pBuffer = nullptr;
            }
        }

        inline void SetBufferSize(uint32_t byteSize)
        {
            if (m_pBuffer != nullptr)
            {
                FlushWrites();
                m_pBufferAllocator->deallocate(m_pBuffer, m_BufferCapacity, Memory::kDefaultAlignment);
            }

            FE_Assert(m_BufferPosition == 0);
            m_BufferCapacity = byteSize;
        }

        inline void EnsureBufferAllocated()
        {
            if (m_pBuffer == nullptr)
                m_pBuffer = Memory::AllocateArray<std::byte>(m_pBufferAllocator, m_BufferCapacity, Memory::kDefaultAlignment);
        }

        inline void SetBufferAllocator(std::pmr::memory_resource* pBufferAllocator)
        {
            FE_CORE_ASSERT(m_pBuffer == nullptr, "Buffer already allocated");

            if (pBufferAllocator == nullptr)
                pBufferAllocator = std::pmr::get_default_resource();
            m_pBufferAllocator = pBufferAllocator;
        }

        inline size_t WriteFromBuffer(festd::span<const std::byte> buffer) final
        {
            EnsureBufferAllocated();

            if (m_BufferPosition + buffer.size() > m_BufferCapacity)
                FlushWrites();

            if (buffer.size() > m_BufferCapacity)
                return WriteImpl(buffer);

            Memory::Copy(buffer, festd::span{ m_pBuffer + m_BufferPosition, buffer.size() });
            m_BufferPosition += buffer.size();
            return buffer.size();
        }

        inline void FlushWrites() final
        {
            const size_t bytesWritten = WriteImpl({ m_pBuffer, m_BufferPosition });
            FE_Assert(bytesWritten == m_BufferPosition);
            m_BufferPosition = 0;
        }
    };
} // namespace FE::IO
