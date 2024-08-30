#pragma once
#include <FeCore/IO/BaseIO.h>

namespace FE::IO::Platform
{
    FileHandle GetStandardFile(StandardDescriptor descriptor);

    ResultCode OpenFile(StringSlice filePath, OpenMode openMode, FileHandle& handle);

    ResultCode GetFileStats(FileHandle fileHandle, FileStats& result);

    FileAttributeFlags GetFileAttributeFlags(StringSlice filePath);

    bool FileExists(StringSlice filePath);

    void CloseFile(FileHandle fileHandle);

    ResultCode ReadFile(FileHandle fileHandle, festd::span<std::byte> buffer, uint32_t& bytesRead);

    ResultCode WriteFile(FileHandle fileHandle, festd::span<const std::byte> buffer, uint32_t& bytesWritten);

    ResultCode SeekFile(FileHandle fileHandle, intptr_t offset, SeekMode seekMode);

    ResultCode TellFile(FileHandle fileHandle, uintptr_t& position);
} // namespace FE::IO::Platform
