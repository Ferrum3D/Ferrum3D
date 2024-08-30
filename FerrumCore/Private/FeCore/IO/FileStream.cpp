#include <FeCore/IO/FileStream.h>
#include <FeCore/IO/Platform/PlatformFile.h>

namespace FE::IO
{
    bool FileStream::SeekAllowed() const
    {
        return true;
    }


    bool FileStream::IsOpen() const
    {
        return m_Handle.IsValid();
    }


    ResultCode FileStream::Seek(intptr_t offset, SeekMode seekMode)
    {
        return Platform::SeekFile(m_Handle, offset, seekMode);
    }


    uintptr_t FileStream::Tell() const
    {
        uintptr_t position;
        FE_IO_ASSERT(Platform::TellFile(m_Handle, position));
        return position;
    }


    size_t FileStream::Length() const
    {
        return m_Stats.ByteSize;
    }


    size_t FileStream::ReadToBuffer(festd::span<std::byte> buffer)
    {
        uint32_t bytesRead;
        FE_IO_ASSERT(Platform::ReadFile(m_Handle, buffer, bytesRead));
        return bytesRead;
    }


    size_t FileStream::WriteImpl(festd::span<const std::byte> buffer)
    {
        uint32_t bytesWritten;
        FE_IO_ASSERT(Platform::WriteFile(m_Handle, buffer, bytesWritten));
        return bytesWritten;
    }


    StringSlice FileStream::GetName()
    {
        return m_Name;
    }


    OpenMode FileStream::GetOpenMode() const
    {
        return m_OpenMode;
    }


    FileStats FileStream::GetStats() const
    {
        return m_Stats;
    }


    void FileStream::Close()
    {
        if (m_Handle)
            Platform::CloseFile(m_Handle);
    }


    ResultCode FileStream::Open(StringSlice fileName, OpenMode openMode)
    {
        ResultCode result = Platform::OpenFile(fileName, openMode, m_Handle);
        if (result != ResultCode::Success)
            return result;

        result = Platform::GetFileStats(m_Handle, m_Stats);
        if (result != ResultCode::Success)
            return result;

        m_Name = fileName;
        m_OpenMode = openMode;
        return ResultCode::Success;
    }


    void FileStream::Open(StandardDescriptor standardDescriptor)
    {
        m_Name = GetStandardDescriptorName(standardDescriptor);
        m_OpenMode = GetStandardDescriptorOpenMode(standardDescriptor);
        m_Handle = Platform::GetStandardFile(standardDescriptor);
    }
} // namespace FE::IO
