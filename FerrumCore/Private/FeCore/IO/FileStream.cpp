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


    ResultCode FileStream::Seek(const intptr_t offset, const SeekMode seekMode)
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


    size_t FileStream::ReadToBuffer(void* buffer, const size_t byteSize)
    {
        size_t bytesRead;
        FE_IO_ASSERT(Platform::ReadFile(m_handle, buffer, byteSize, bytesRead));
        return bytesRead;
    }


    size_t FileStream::WriteImpl(const void* buffer, const size_t byteSize)
    {
        size_t bytesWritten;
        FE_IO_ASSERT(Platform::WriteFile(m_handle, buffer, byteSize, bytesWritten));
        return bytesWritten;
    }


    festd::string_view FileStream::GetName()
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
        {
            FlushWrites();
            Platform::CloseFile(m_handle);
        }
    }


    ResultCode FileStream::Open(const festd::string_view fileName, const OpenMode openMode)
    {
        FE_PROFILER_ZONE_TEXT("%.*s", fileName.size(), fileName.data());

        ResultCode result = Platform::OpenFile(fileName, openMode, m_handle);
        if (result != ResultCode::kSuccess)
            return result;

        result = Platform::GetFileStats(m_handle, m_stats);
        if (result != ResultCode::kSuccess)
            return result;

        m_name = fileName;
        m_openMode = openMode;
        return ResultCode::kSuccess;
    }


    void FileStream::Open(const StandardDescriptor standardDescriptor)
    {
        m_name = GetStandardDescriptorName(standardDescriptor);
        m_openMode = GetStandardDescriptorOpenMode(standardDescriptor);
        m_handle = Platform::GetStandardFile(standardDescriptor);
    }
} // namespace FE::IO
