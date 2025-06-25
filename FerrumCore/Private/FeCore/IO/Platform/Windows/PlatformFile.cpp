#include <FeCore/IO/Platform/PlatformFile.h>
#include <FeCore/Platform/Windows/Common.h>
#include <festd/vector.h>

namespace FE::Platform
{
    namespace
    {
        HANDLE HandleCast(const FileHandle handle)
        {
            return reinterpret_cast<HANDLE>(handle.m_value);
        }


        DWORD GetFileAccessFlags(const IO::OpenMode openMode)
        {
            switch (openMode)
            {
            case IO::OpenMode::kNone:
                return 0;
            case IO::OpenMode::kReadOnly:
                return GENERIC_READ;
            case IO::OpenMode::kWriteOnly:
            case IO::OpenMode::kCreate:
            case IO::OpenMode::kCreateNew:
                return GENERIC_WRITE;
            case IO::OpenMode::kAppend:
            case IO::OpenMode::kTruncate:
            case IO::OpenMode::kReadWrite:
                return GENERIC_READ | GENERIC_WRITE;
            default:
                return 0;
            }
        }


        DWORD GetFileShareMode(const IO::OpenMode openMode)
        {
            if (openMode == IO::OpenMode::kReadOnly)
                return FILE_SHARE_READ;

            return 0;
        }


        DWORD GetFileCreationDisposition(const IO::OpenMode openMode)
        {
            switch (openMode)
            {
            case IO::OpenMode::kNone:
                return 0;
            case IO::OpenMode::kReadOnly:
            case IO::OpenMode::kWriteOnly:
            case IO::OpenMode::kReadWrite:
            case IO::OpenMode::kAppend:
                return OPEN_EXISTING;
            case IO::OpenMode::kCreate:
                return CREATE_ALWAYS;
            case IO::OpenMode::kCreateNew:
                return CREATE_NEW;
            case IO::OpenMode::kTruncate:
                return TRUNCATE_EXISTING;
            default:
                return 0;
            }
        }


        DWORD GetFileSeekMode(const IO::SeekMode seekMode)
        {
            switch (seekMode)
            {
            case IO::SeekMode::kBegin:
                return FILE_BEGIN;
            case IO::SeekMode::kCurrent:
                return FILE_CURRENT;
            case IO::SeekMode::kEnd:
                return FILE_END;
            default:
                return 0;
            }
        }
    } // namespace


    FileHandle GetStandardFile(const IO::StandardDescriptor descriptor)
    {
        DWORD stdDescriptor;
        switch (descriptor)
        {
        case IO::StandardDescriptor::kStdin:
            stdDescriptor = STD_INPUT_HANDLE;
            break;
        case IO::StandardDescriptor::kStdout:
            stdDescriptor = STD_OUTPUT_HANDLE;
            break;
        case IO::StandardDescriptor::kStderr:
            stdDescriptor = STD_ERROR_HANDLE;
            break;
        default:
            return FileHandle{};
        }

        return FileHandle::FromPointer(GetStdHandle(stdDescriptor));
    }


    IO::ResultCode OpenFile(const festd::string_view filePath, const IO::OpenMode openMode, FileHandle& handle)
    {
        FE_PROFILER_ZONE_TEXT("%.*s", filePath.size(), filePath.data());

        const WideString widePath{ filePath };
        if (widePath.m_value.empty())
            return IO::ResultCode::kInvalidArgument;

        const DWORD desiredFileAccessFlags = GetFileAccessFlags(openMode);
        const DWORD fileShareMode = GetFileShareMode(openMode);
        const DWORD fileCreationDisposition = GetFileCreationDisposition(openMode);

        const HANDLE nativeFileHandle = CreateFileW(widePath.m_value.data(),
                                                    desiredFileAccessFlags,
                                                    fileShareMode,
                                                    nullptr,
                                                    fileCreationDisposition,
                                                    FILE_ATTRIBUTE_NORMAL,
                                                    nullptr);

        if (nativeFileHandle == INVALID_HANDLE_VALUE)
            return ConvertWin32IOError(GetLastError());

        handle = FileHandle::FromPointer(nativeFileHandle);
        return IO::ResultCode::kSuccess;
    }


    IO::ResultCode GetFileStats(const FileHandle fileHandle, IO::FileStats& result)
    {
        FE_PROFILER_ZONE();

        const HANDLE hFile = HandleCast(fileHandle);
        FILETIME creationFT, accessFT, writeFT;
        if (!GetFileTime(hFile, &creationFT, &accessFT, &writeFT))
            return ConvertWin32IOError(GetLastError());

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile, &fileSize))
            return ConvertWin32IOError(GetLastError());

        result.m_creationTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(creationFT));
        result.m_accessTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(accessFT));
        result.m_modificationTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(writeFT));
        result.m_byteSize = static_cast<uint64_t>(fileSize.QuadPart);
        return IO::ResultCode::kSuccess;
    }


    IO::FileAttributeFlags GetFileAttributeFlags(const festd::string_view filePath)
    {
        FE_PROFILER_ZONE_TEXT("%.*s", filePath.size(), filePath.data());

        const WideString widePath{ filePath };
        if (widePath.m_value.empty())
            return IO::FileAttributeFlags::kInvalid;

        const DWORD attributes = GetFileAttributesW(widePath.m_value.data());
        return ConvertFileAttributeFlags(attributes);
    }


    bool FileExists(const festd::string_view filePath)
    {
        FE_PROFILER_ZONE();
        const IO::FileAttributeFlags attributes = GetFileAttributeFlags(filePath);
        return attributes != IO::FileAttributeFlags::kInvalid && attributes != IO::FileAttributeFlags::kDirectory;
    }


    void CloseFile(const FileHandle fileHandle)
    {
        FE_PROFILER_ZONE();
        CloseHandle(HandleCast(fileHandle));
    }


    IO::ResultCode ReadFile(const FileHandle fileHandle, void* buffer, const size_t byteSize, size_t& bytesRead)
    {
        FE_PROFILER_ZONE_TEXT("%" PRIu64, byteSize);

        bytesRead = 0;
        while (bytesRead < byteSize)
        {
            const size_t bytesToRead = Math::Min<size_t>(byteSize - bytesRead, Constants::kMaxValue<DWORD>);

            DWORD dwBytesRead = 0;
            if (!::ReadFile(HandleCast(fileHandle),
                            static_cast<std::byte*>(buffer) + bytesRead,
                            static_cast<DWORD>(bytesToRead),
                            &dwBytesRead,
                            nullptr))
            {
                return ConvertWin32IOError(GetLastError());
            }

            if (dwBytesRead == 0)
                break;

            bytesRead += dwBytesRead;
        }

        return IO::ResultCode::kSuccess;
    }


    IO::ResultCode WriteFile(const FileHandle fileHandle, const void* buffer, const size_t byteSize, size_t& bytesWritten)
    {
        FE_PROFILER_ZONE_TEXT("%" PRIu64, byteSize);

        bytesWritten = 0;
        while (bytesWritten < byteSize)
        {
            const size_t bytesToWrite = Math::Min<size_t>(byteSize - bytesWritten, Constants::kMaxValue<DWORD>);

            DWORD dwBytesWritten = 0;
            if (!::WriteFile(HandleCast(fileHandle),
                             static_cast<const std::byte*>(buffer) + bytesWritten,
                             static_cast<DWORD>(bytesToWrite),
                             &dwBytesWritten,
                             nullptr))
            {
                return ConvertWin32IOError(GetLastError());
            }

            bytesWritten += dwBytesWritten;
        }

        return IO::ResultCode::kSuccess;
    }


    IO::ResultCode SeekFile(const FileHandle fileHandle, const intptr_t offset, const IO::SeekMode seekMode)
    {
        FE_PROFILER_ZONE();

        LARGE_INTEGER distance;
        distance.QuadPart = offset;
        if (!SetFilePointerEx(HandleCast(fileHandle), distance, nullptr, GetFileSeekMode(seekMode)))
        {
            return ConvertWin32IOError(GetLastError());
        }

        return IO::ResultCode::kSuccess;
    }


    IO::ResultCode TellFile(const FileHandle fileHandle, uintptr_t& position)
    {
        FE_PROFILER_ZONE();

        LARGE_INTEGER distance;
        distance.QuadPart = 0;
        if (!SetFilePointerEx(HandleCast(fileHandle), distance, reinterpret_cast<PLARGE_INTEGER>(&position), FILE_CURRENT))
        {
            return ConvertWin32IOError(GetLastError());
        }

        return IO::ResultCode::kSuccess;
    }
} // namespace FE::Platform
