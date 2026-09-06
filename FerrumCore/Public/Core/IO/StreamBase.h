#pragma once
#include <Core/IO/IStream.h>
#include <Core/Logging/Trace.h>

namespace FE::IO
{
    struct StreamBase : public IStream
    {
        FE_RTTI("2F74FF8D-4D81-44BE-962A-9D30669E03C8");

        ~StreamBase() override = default;

        bool WriteAllowed() const override
        {
            return IsWriteAllowed(GetOpenMode());
        }

        bool ReadAllowed() const override
        {
            return IsReadAllowed(GetOpenMode());
        }

        size_t WriteFromStream(IStream* stream, const size_t size) override
        {
            FE_Assert(stream != nullptr);
            FE_Assert(stream->ReadAllowed(), "Source stream was write-only");
            FE_Assert(WriteAllowed(), "Destination stream was read-only");
            FE_Assert(stream != this, "Destination and source streams are the same");

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
            FE_Assert(false, "Not supported");
            return {};
        }
    };


    struct BufferedStream : public StreamBase
    {
        FE_RTTI("CCAD9E96-A9C7-4543-9414-1A0E00E8D5B6");

        BufferedStream(const BufferedStream&) = delete;
        BufferedStream& operator=(const BufferedStream&) = delete;

        BufferedStream(BufferedStream&& other) noexcept
        {
            swap(*this, other);
        }

        BufferedStream& operator=(BufferedStream&& other) noexcept
        {
            swap(*this, other);
            return *this;
        }

        ~BufferedStream() override
        {
            if (m_buffer)
            {
                m_bufferAllocator->deallocate(m_buffer, m_bufferCapacity, Memory::kDefaultAlignment);
                m_buffer = nullptr;
            }
        }

        void SetBufferSize(const size_t byteSize)
        {
            if (m_buffer != nullptr && m_bufferCapacity != byteSize)
            {
                FlushWrites();
                m_bufferAllocator->deallocate(m_buffer, m_bufferCapacity, Memory::kDefaultAlignment);
                m_buffer = nullptr;
            }

            FE_Assert(m_bufferPosition == 0);
            m_bufferCapacity = byteSize;
        }

        void EnsureBufferAllocated()
        {
            if (m_buffer == nullptr && m_bufferCapacity > 0)
                m_buffer = Memory::AllocateArray<std::byte>(m_bufferAllocator, m_bufferCapacity, Memory::kDefaultAlignment);
        }

        void SetBufferAllocator(std::pmr::memory_resource* pBufferAllocator)
        {
            FE_Assert(m_buffer == nullptr, "Buffer already allocated");

            if (pBufferAllocator == nullptr)
                pBufferAllocator = std::pmr::get_default_resource();
            m_bufferAllocator = pBufferAllocator;
        }

        size_t WriteFromBuffer(const void* buffer, const size_t byteSize) final
        {
            EnsureBufferAllocated();

            if (m_bufferPosition + byteSize > m_bufferCapacity)
                FlushWrites();

            if (byteSize > m_bufferCapacity)
                return WriteImpl(buffer, byteSize);

            memcpy(m_buffer + m_bufferPosition, buffer, byteSize);
            m_bufferPosition += byteSize;
            return byteSize;
        }

        void FlushWrites() override
        {
            if (m_bufferPosition > 0)
            {
                const size_t bytesWritten = WriteImpl(m_buffer, m_bufferPosition);
                FE_Assert(bytesWritten == m_bufferPosition);
                m_bufferPosition = 0;
            }
        }

        friend void swap(BufferedStream& lhs, BufferedStream& rhs) noexcept
        {
            festd::swap(lhs.m_bufferAllocator, rhs.m_bufferAllocator);
            festd::swap(lhs.m_buffer, rhs.m_buffer);
            festd::swap(lhs.m_bufferCapacity, rhs.m_bufferCapacity);
            festd::swap(lhs.m_bufferPosition, rhs.m_bufferPosition);
        }

    protected:
        std::pmr::memory_resource* m_bufferAllocator = nullptr;
        std::byte* m_buffer = nullptr;
        size_t m_bufferCapacity = 4 * 1024;
        size_t m_bufferPosition = 0;

        explicit BufferedStream(std::pmr::memory_resource* pBufferAllocator)
        {
            SetBufferAllocator(pBufferAllocator);
        }

        virtual size_t WriteImpl(const void* buffer, size_t byteSize) = 0;
    };


    struct ReadOnlyMemoryStream final : public StreamBase
    {
        ReadOnlyMemoryStream() = default;
        ReadOnlyMemoryStream(const void* buffer, const size_t bufferByteSize)
        {
            OpenInPlace(buffer, bufferByteSize);
        }

        explicit ReadOnlyMemoryStream(const festd::span<const std::byte> buffer)
            : ReadOnlyMemoryStream(buffer.data(), buffer.size_bytes())
        {
        }

        void OpenInPlace(const void* buffer, const size_t bufferByteSize)
        {
            m_buffer = static_cast<const std::byte*>(buffer);
            m_bufferSize = bufferByteSize;
            m_position = 0;
        }

        [[nodiscard]] bool SeekAllowed() const override
        {
            return true;
        }

        [[nodiscard]] bool IsOpen() const override
        {
            return m_buffer != nullptr;
        }

        ResultCode Seek(const intptr_t offset, const SeekMode seekMode) override
        {
            intptr_t newPosition;
            switch (seekMode)
            {
            default:
                FE_DebugBreak();
                [[fallthrough]];

            case SeekMode::kBegin:
                newPosition = offset;
                break;

            case SeekMode::kEnd:
                newPosition = static_cast<intptr_t>(m_bufferSize) + offset;
                break;

            case SeekMode::kCurrent:
                newPosition = static_cast<intptr_t>(m_position) + offset;
                break;
            }

            if (newPosition >= 0 && newPosition <= m_bufferSize)
                m_position = newPosition;

            return ResultCode::kSuccess;
        }

        [[nodiscard]] uintptr_t Tell() const override
        {
            return m_position;
        }

        [[nodiscard]] size_t Length() const override
        {
            return m_bufferSize;
        }

        size_t ReadToBuffer(void* buffer, const size_t byteSize) override
        {
            const size_t bytesToRead = Math::Min(byteSize, m_bufferSize - m_position);
            memcpy(buffer, m_buffer + m_position, bytesToRead);
            return bytesToRead;
        }

        size_t WriteFromBuffer(const void* buffer, size_t byteSize) override
        {
            FE_Unused(buffer);
            FE_Unused(byteSize);
            FE_DebugBreak();
            return 0;
        }

        [[nodiscard]] festd::string_view GetName() override
        {
            return "ReadOnlyMemoryStream";
        }

        [[nodiscard]] OpenMode GetOpenMode() const override
        {
            return OpenMode::kReadOnly;
        }

        void Close() override {}

    private:
        const std::byte* m_buffer = nullptr;
        size_t m_bufferSize = 0;
        uintptr_t m_position = 0;

        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }
    };
} // namespace FE::IO
