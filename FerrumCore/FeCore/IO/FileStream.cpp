#include <FeCore/IO/FileStream.h>

#include <utility>

namespace FE::IO
{
    bool FileStream::WriteAllowed() const noexcept
    {
        return IsWriteAllowed(GetOpenMode());
    }

    bool FileStream::ReadAllowed() const noexcept
    {
        return IsReadAllowed(GetOpenMode());
    }

    bool FileStream::SeekAllowed() const noexcept
    {
        return true;
    }

    bool FileStream::IsOpen() const
    {
        return m_Handle->IsOpen();
    }

    ResultCode FileStream::Seek(SSize offset, SeekMode seekMode)
    {
        return m_Handle->Seek(offset, seekMode);
    }

    USize FileStream::Tell() const
    {
        return m_Handle->Tell();
    }

    USize FileStream::Length() const
    {
        return m_Handle->Length();
    }

    USize FileStream::ReadToBuffer(void* buffer, USize size)
    {
        return m_Handle->Read(buffer, size);
    }

    USize FileStream::WriteFromBuffer(const void* buffer, USize size)
    {
        return m_Handle->Write(buffer, size);
    }

    StringSlice FileStream::GetName()
    {
        return m_Handle->GetName();
    }

    OpenMode FileStream::GetOpenMode() const
    {
        return m_Handle->GetOpenMode();
    }

    void FileStream::Close()
    {
        m_Handle->Close();
    }

    ResultCode FileStream::Open(StringSlice fileName, OpenMode openMode)
    {
        return m_Handle->Open(fileName, openMode);
    }

    FileStream::FileStream(const Shared<FileHandle>& file)
        : m_Handle(file)
    {
    }

    FileStream::FileStream(Shared<FileHandle>&& file)
        : m_Handle(std::move(file))
    {
    }
}
