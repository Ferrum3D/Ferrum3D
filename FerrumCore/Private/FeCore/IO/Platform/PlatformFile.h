#pragma once
#include <FeCore/IO/BaseIO.h>

namespace FE::Platform
{
    FileHandle GetStandardFile(IO::StandardDescriptor descriptor);

    IO::ResultCode OpenFile(festd::string_view filePath, IO::OpenMode openMode, FileHandle& handle);

    IO::ResultCode GetFileStats(FileHandle fileHandle, IO::FileStats& result);

    IO::FileAttributeFlags GetFileAttributeFlags(festd::string_view filePath);

    bool FileExists(festd::string_view filePath);

    void CloseFile(FileHandle fileHandle);

    IO::ResultCode ReadFile(FileHandle fileHandle, void* buffer, size_t byteSize, size_t& bytesRead);

    IO::ResultCode WriteFile(FileHandle fileHandle, const void* buffer, size_t byteSize, size_t& bytesWritten);

    IO::ResultCode SeekFile(FileHandle fileHandle, intptr_t offset, IO::SeekMode seekMode);

    IO::ResultCode TellFile(FileHandle fileHandle, uintptr_t& position);
} // namespace FE::Platform
