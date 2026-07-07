#include <Core/IO/FileStream.h>
#include <Core/IO/Platform/PlatformFile.h>

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
            BufferedStream::FlushWrites();
            Platform::CloseFile(m_handle);
            m_handle.Reset();
        }
    }


    void FileStream::FlushWrites()
    {
        BufferedStream::FlushWrites();
        Platform::FlushFile(m_handle);
    }


    festd::expected<Rc<FileStream>, ResultCode> FileStream::Open(const festd::string_view fileName, const OpenMode openMode,
                                                                 std::pmr::memory_resource* bufferAllocator)
    {
        FE_PROFILER_ZONE_TEXT("%.*s", fileName.size(), fileName.data());

        Platform::FileHandle handle;
        ResultCode result = Platform::OpenFile(fileName, openMode, handle);
        if (result != ResultCode::kSuccess)
            return festd::unexpected(result);

        FileStats stats;
        result = Platform::GetFileStats(handle, stats);
        if (result != ResultCode::kSuccess)
            return festd::unexpected(result);

        Rc<FileStream> stream = Memory::DefaultNew<FileStream>(bufferAllocator);
        stream->m_name = fileName;
        stream->m_handle = handle;
        stream->m_stats = stats;
        stream->m_openMode = openMode;
        return stream;
    }


    void FileStream::OpenInPlace(StandardDescriptor standardDescriptor)
    {
        m_name = GetStandardDescriptorName(standardDescriptor);
        m_openMode = GetStandardDescriptorOpenMode(standardDescriptor);
        m_handle = Platform::GetStandardFile(standardDescriptor);
    }


    FileStream::FileStream(FileStream&& other) noexcept
        : BufferedStream(nullptr)
    {
        swap(*this, other);
    }


    FileStream& FileStream::operator=(FileStream&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }


    Rc<FileStream> FileStream::Open(const StandardDescriptor standardDescriptor, std::pmr::memory_resource* bufferAllocator)
    {
        Rc<FileStream> stream = Memory::DefaultNew<FileStream>(bufferAllocator);
        stream->OpenInPlace(standardDescriptor);
        return stream;
    }


    void swap(FileStream& lhs, FileStream& rhs) noexcept
    {
        swap(static_cast<BufferedStream&>(lhs), static_cast<BufferedStream&>(rhs));
        festd::swap(lhs.m_name, rhs.m_name);
        festd::swap(lhs.m_handle, rhs.m_handle);
        festd::swap(lhs.m_stats, rhs.m_stats);
        festd::swap(lhs.m_openMode, rhs.m_openMode);
    }
} // namespace FE::IO
