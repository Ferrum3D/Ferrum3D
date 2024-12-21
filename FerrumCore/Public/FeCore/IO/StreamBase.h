#pragma once
#include <FeCore/IO/IStream.h>
#include <FeCore/Logging/Trace.h>

namespace FE::IO
{
    struct StreamBase : public IStream
    {
        FE_RTTI_Class(StreamBase, "2F74FF8D-4D81-44BE-962A-9D30669E03C8");

        ~StreamBase() override = default;

        bool WriteAllowed() const override
        {
            return IsWriteAllowed(GetOpenMode());
        }

        bool ReadAllowed() const override
        {
            return IsReadAllowed(GetOpenMode());
        }

        size_t WriteFromStream(IStream* stream, size_t size) override
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

        void FlushWrites() override {}

        FileStats GetStats() const override
        {
            FE_AssertMsg(false, "Not supported");
            return {};
        }
    };


    struct BufferedStream : public StreamBase
    {
        ~BufferedStream()
        {
            if (m_buffer)
            {
                m_bufferAllocator->deallocate(m_buffer, m_bufferCapacity, Memory::kDefaultAlignment);
                m_buffer = nullptr;
            }
        }

        void SetBufferSize(uint32_t byteSize)
        {
            if (m_buffer != nullptr)
            {
                FlushWrites();
                m_bufferAllocator->deallocate(m_buffer, m_bufferCapacity, Memory::kDefaultAlignment);
            }

            FE_Assert(m_bufferPosition == 0);
            m_bufferCapacity = byteSize;
        }

        void EnsureBufferAllocated()
        {
            if (m_buffer == nullptr)
                m_buffer = Memory::AllocateArray<std::byte>(m_bufferAllocator, m_bufferCapacity, Memory::kDefaultAlignment);
        }

        void SetBufferAllocator(std::pmr::memory_resource* pBufferAllocator)
        {
            FE_CORE_ASSERT(m_buffer == nullptr, "Buffer already allocated");

            if (pBufferAllocator == nullptr)
                pBufferAllocator = std::pmr::get_default_resource();
            m_bufferAllocator = pBufferAllocator;
        }

        size_t WriteFromBuffer(festd::span<const std::byte> buffer) final
        {
            EnsureBufferAllocated();

            if (m_bufferPosition + buffer.size() > m_bufferCapacity)
                FlushWrites();

            if (buffer.size() > m_bufferCapacity)
                return WriteImpl(buffer);

            Memory::Copy(buffer, festd::span{ m_buffer + m_bufferPosition, buffer.size() });
            m_bufferPosition += buffer.size();
            return buffer.size();
        }

        void FlushWrites() final
        {
            const size_t bytesWritten = WriteImpl({ m_buffer, m_bufferPosition });
            FE_Assert(bytesWritten == m_bufferPosition);
            m_bufferPosition = 0;
        }

    protected:
        std::pmr::memory_resource* m_bufferAllocator = nullptr;
        std::byte* m_buffer = nullptr;
        uint32_t m_bufferCapacity = 1024;
        uint32_t m_bufferPosition = 0;

        BufferedStream(std::pmr::memory_resource* pBufferAllocator)
        {
            SetBufferAllocator(pBufferAllocator);
        }

        virtual size_t WriteImpl(festd::span<const std::byte> buffer) = 0;
    };
} // namespace FE::IO
