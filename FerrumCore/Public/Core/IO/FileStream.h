#pragma once
#include <Core/IO/StreamBase.h>

namespace FE::IO
{
    struct FileStream final : public BufferedStream
    {
        FE_RTTI("25BF9144-CF17-4085-B0D4-F269D5F7CCB7");

        explicit FileStream(std::pmr::memory_resource* bufferAllocator = nullptr)
            : BufferedStream(bufferAllocator)
        {
        }

        FileStream(const FileStream&) = delete;
        FileStream& operator=(const FileStream&) = delete;

        FileStream(FileStream&& other) noexcept;
        FileStream& operator=(FileStream&& other) noexcept;

        ~FileStream() override
        {
            Close();
        }

        [[nodiscard]] static Rc<FileStream> Open(StandardDescriptor standardDescriptor,
                                                 std::pmr::memory_resource* bufferAllocator = nullptr);
        [[nodiscard]] static festd::expected<Rc<FileStream>, ResultCode> Open(
            festd::string_view fileName, OpenMode openMode, std::pmr::memory_resource* bufferAllocator = nullptr);

        void OpenInPlace(StandardDescriptor standardDescriptor);

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
        void FlushWrites() override;

        friend void swap(FileStream& lhs, FileStream& rhs) noexcept;

    private:
        Path m_name;
        Platform::FileHandle m_handle;
        FileStats m_stats{};
        OpenMode m_openMode = OpenMode::kNone;

        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        size_t WriteImpl(const void* buffer, size_t byteSize) override;
    };
} // namespace FE::IO
