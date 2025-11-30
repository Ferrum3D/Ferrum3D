#pragma once
#include <FeCore/IO/StreamBase.h>

namespace FE::IO
{
    struct FileStream final : public BufferedStream
    {
        FE_RTTI("25BF9144-CF17-4085-B0D4-F269D5F7CCB7");

        explicit FileStream(std::pmr::memory_resource* bufferAllocator = nullptr)
            : BufferedStream(bufferAllocator)
        {
        }

        ~FileStream() override
        {
            Close();
        }

        ResultCode Open(festd::string_view fileName, OpenMode openMode);
        void Open(StandardDescriptor standardDescriptor);

        [[nodiscard]] bool SeekAllowed() const override;
        [[nodiscard]] bool IsOpen() const override;
        ResultCode Seek(intptr_t offset, SeekMode seekMode) override;
        [[nodiscard]] uintptr_t Tell() const override;
        [[nodiscard]] size_t Length() const override;
        size_t ReadToBuffer(void* buffer, size_t byteSize) override;
        festd::string_view GetName() override;
        [[nodiscard]] OpenMode GetOpenMode() const override;
        [[nodiscard]] FileStats GetStats() const override;
        void Close() override;

    private:
        Path m_name;
        Platform::FileHandle m_handle;
        FileStats m_stats{};
        OpenMode m_openMode = OpenMode::kNone;

        size_t WriteImpl(const void* buffer, size_t byteSize) override;
    };
} // namespace FE::IO
