#pragma once
#include <FeCore/IO/StreamBase.h>
#include <FeCore/IO/FileHandle.h>

namespace FE::IO
{
    class FileStream : public StreamBase
    {
        Rc<FileHandle> m_Handle;

    public:
        explicit FileStream(const Rc<FileHandle>& file);
        explicit FileStream(Rc<FileHandle>&& file);

        ~FileStream() override = default;

        ResultCode Open(StringSlice fileName, OpenMode openMode);

        [[nodiscard]] bool WriteAllowed() const noexcept override;
        [[nodiscard]] bool ReadAllowed() const noexcept override;
        [[nodiscard]] bool SeekAllowed() const noexcept override;
        [[nodiscard]] bool IsOpen() const override;
        ResultCode Seek(ptrdiff_t offset, SeekMode seekMode) override;
        [[nodiscard]] size_t Tell() const override;
        [[nodiscard]] size_t Length() const override;
        size_t ReadToBuffer(void* buffer, size_t size) override;
        size_t WriteFromBuffer(const void* buffer, size_t size) override;
        StringSlice GetName() override;
        [[nodiscard]] OpenMode GetOpenMode() const override;
        void Close() override;
    };
}
