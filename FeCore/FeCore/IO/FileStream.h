#pragma once
#include <FeCore/IO/StreamBase.h>
#include <FeCore/IO/FileHandle.h>

namespace FE::IO
{
    class FileStream : public StreamBase
    {
        FileHandle* m_Handle;
        OpenMode m_OpenMode;

    public:
        explicit FileStream(FileHandle* file);
        ~FileStream() override = default;

        ResultCode Open(StringSlice fileName, OpenMode openMode);

        [[nodiscard]] bool WriteAllowed() const noexcept override;
        [[nodiscard]] bool ReadAllowed() const noexcept override;
        [[nodiscard]] bool SeekAllowed() const noexcept override;
        [[nodiscard]] bool IsOpen() const override;
        ResultCode Seek(SSize offset, SeekMode seekMode) override;
        [[nodiscard]] USize Tell() const override;
        [[nodiscard]] USize Length() const override;
        USize ReadToBuffer(void* buffer, USize size) override;
        USize WriteFromBuffer(const void* buffer, USize size) override;
        StringSlice GetName() override;
        [[nodiscard]] OpenMode GetOpenMode() const override;
        void Close() override;
    };
}
