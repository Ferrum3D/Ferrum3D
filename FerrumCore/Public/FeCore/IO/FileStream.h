#pragma once
#include <FeCore/IO/StreamBase.h>

namespace FE::IO
{
    class FileStream : public BufferedStream
    {
        FixedPath m_Name;
        Platform::FileHandle m_Handle;
        FileStats m_Stats;
        OpenMode m_OpenMode = OpenMode::kNone;

        size_t WriteImpl(festd::span<const std::byte> buffer) override;

    public:
        inline FileStream(std::pmr::memory_resource* pBufferAllocator = nullptr)
            : BufferedStream(pBufferAllocator)
        {
        }

        inline ~FileStream() override
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
    };
} // namespace FE::IO
