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
        return m_handle.IsValid();
    }


    ResultCode FileStream::Seek(intptr_t offset, SeekMode seekMode)
    {
        return Platform::SeekFile(m_handle, offset, seekMode);
    }


    uintptr_t FileStream::Tell() const
    {
        uintptr_t position;
        FE_IO_ASSERT(Platform::TellFile(m_handle, position));
        return position;
    }


    size_t FileStream::Length() const
    {
        return m_stats.m_byteSize;
    }


    size_t FileStream::ReadToBuffer(festd::span<std::byte> buffer)
    {
        uint32_t bytesRead;
        FE_IO_ASSERT(Platform::ReadFile(m_handle, buffer, bytesRead));
        return bytesRead;
    }


    size_t FileStream::WriteImpl(festd::span<const std::byte> buffer)
    {
        uint32_t bytesWritten;
        FE_IO_ASSERT(Platform::WriteFile(m_handle, buffer, bytesWritten));
        return bytesWritten;
    }


    StringSlice FileStream::GetName()
    {
        return m_name;
    }


    OpenMode FileStream::GetOpenMode() const
    {
        return m_openMode;
    }


    FileStats FileStream::GetStats() const
    {
        return m_stats;
    }


    void FileStream::Close()
    {
        if (m_handle)
            Platform::CloseFile(m_handle);
    }


    ResultCode FileStream::Open(StringSlice fileName, OpenMode openMode)
    {
        ResultCode result = Platform::OpenFile(fileName, openMode, m_handle);
        if (result != ResultCode::Success)
            return result;

        result = Platform::GetFileStats(m_handle, m_stats);
        if (result != ResultCode::Success)
            return result;

        m_name = fileName;
        m_openMode = openMode;
        return ResultCode::Success;
    }


    void FileStream::Open(StandardDescriptor standardDescriptor)
    {
        m_name = GetStandardDescriptorName(standardDescriptor);
        m_openMode = GetStandardDescriptorOpenMode(standardDescriptor);
        m_handle = Platform::GetStandardFile(standardDescriptor);
    }
} // namespace FE::IO
