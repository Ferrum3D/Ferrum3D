#pragma once
#include <Core/IO/StreamBase.h>
#include <festd/vector.h>

namespace FE::IO
{
    struct WriteOnlyMemoryStream final : public StreamBase
    {
        WriteOnlyMemoryStream() = default;

        [[nodiscard]] bool SeekAllowed() const override
        {
            return false;
        }

        [[nodiscard]] bool IsOpen() const override
        {
            return true;
        }

        ResultCode Seek(intptr_t, SeekMode) override
        {
            return ResultCode::kNotSupported;
        }

        [[nodiscard]] uintptr_t Tell() const override
        {
            return m_totalByteSize;
        }

        [[nodiscard]] size_t Length() const override
        {
            return m_totalByteSize;
        }

        size_t ReadToBuffer(void* buffer, size_t byteSize) override;

        size_t WriteFromBuffer(const void* buffer, size_t byteSize) override;

        [[nodiscard]] festd::string_view GetName() override
        {
            return "WriteOnlyMemoryStream";
        }

        [[nodiscard]] OpenMode GetOpenMode() const override
        {
            return OpenMode::kWriteOnly;
        }

        void Close() override {}

        void DumpAll(festd::pmr::vector<std::byte>& output);

    private:
        static constexpr size_t kPageCapacity = 64 * 1024;

        struct Page
        {
            std::byte* m_data = nullptr;
            size_t m_byteSize = 0;
        };

        festd::inline_vector<Page> m_pages;
        size_t m_totalByteSize = 0;

        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }
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
            FE_Assert(buffer != nullptr || bufferByteSize == 0);
            m_buffer = static_cast<const std::byte*>(buffer);
            m_bufferSize = bufferByteSize;
            m_position = 0;
            m_isOpen = true;
        }

        [[nodiscard]] bool SeekAllowed() const override
        {
            return true;
        }

        [[nodiscard]] bool IsOpen() const override
        {
            return m_isOpen;
        }

        ResultCode Seek(const intptr_t offset, const SeekMode seekMode) override
        {
            if (!m_isOpen)
                return ResultCode::kInvalidSeek;

            uintptr_t basePosition;
            switch (seekMode)
            {
            default:
                return ResultCode::kInvalidSeek;

            case SeekMode::kBegin:
                basePosition = 0;
                break;

            case SeekMode::kEnd:
                basePosition = m_bufferSize;
                break;

            case SeekMode::kCurrent:
                basePosition = m_position;
                break;
            }

            uintptr_t newPosition;
            if (offset >= 0)
            {
                const auto positiveOffset = static_cast<uintptr_t>(offset);
                if (positiveOffset > m_bufferSize - basePosition)
                    return ResultCode::kInvalidSeek;

                newPosition = basePosition + positiveOffset;
            }
            else
            {
                const auto negativeOffset = static_cast<uintptr_t>(-(offset + 1)) + 1;
                if (negativeOffset > basePosition)
                    return ResultCode::kInvalidSeek;

                newPosition = basePosition - negativeOffset;
            }

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
            if (!m_isOpen || byteSize == 0 || m_position == m_bufferSize)
                return 0;

            const size_t bytesToRead = Math::Min(byteSize, m_bufferSize - m_position);
            memcpy(buffer, m_buffer + m_position, bytesToRead);
            m_position += bytesToRead;
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

        void Close() override
        {
            m_buffer = nullptr;
            m_bufferSize = 0;
            m_position = 0;
            m_isOpen = false;
        }

    private:
        const std::byte* m_buffer = nullptr;
        size_t m_bufferSize = 0;
        uintptr_t m_position = 0;
        bool m_isOpen = false;

        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }
    };
} // namespace FE::IO
