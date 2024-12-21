#pragma once
#include <FeCore/IO/StreamBase.h>

namespace FE::IO
{
    struct FileStream final : public BufferedStream
    {
        explicit FileStream(std::pmr::memory_resource* pBufferAllocator = nullptr)
            : BufferedStream(pBufferAllocator)
        {
        }

        ~FileStream() override
        {
            Close();
        }

        ResultCode Open(StringSlice fileName, OpenMode openMode);
        void Open(StandardDescriptor standardDescriptor);

        [[nodiscard]] bool SeekAllowed() const override;
        [[nodiscard]] bool IsOpen() const override;
        ResultCode Seek(intptr_t offset, SeekMode seekMode) override;
        [[nodiscard]] uintptr_t Tell() const override;
        [[nodiscard]] size_t Length() const override;
        size_t ReadToBuffer(festd::span<std::byte> buffer) override;
        StringSlice GetName() override;
        [[nodiscard]] OpenMode GetOpenMode() const override;
        [[nodiscard]] virtual FileStats GetStats() const override;
        void Close() override;

    private:
        FixedPath m_name;
        Platform::FileHandle m_handle;
        FileStats m_stats{};
        OpenMode m_openMode = OpenMode::kNone;

        size_t WriteImpl(festd::span<const std::byte> buffer) override;
    };
} // namespace FE::IO
