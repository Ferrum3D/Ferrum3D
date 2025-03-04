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


        IO::ResultCode ConvertWin32Error(const DWORD error)
        {
            switch (error)
            {
            case ERROR_ALREADY_EXISTS:
            case ERROR_FILE_EXISTS:
                return IO::ResultCode::FileExists;
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
                return IO::ResultCode::NoFileOrDirectory;
            case ERROR_ACCESS_DENIED:
                return IO::ResultCode::PermissionDenied;
            case ERROR_SHARING_VIOLATION:
            case ERROR_INVALID_PARAMETER:
                return IO::ResultCode::InvalidArgument;
            case ERROR_FILE_TOO_LARGE:
                return IO::ResultCode::FileTooLarge;
            case ERROR_TOO_MANY_OPEN_FILES:
                return IO::ResultCode::TooManyOpenFiles;
            case ERROR_SEEK:
                return IO::ResultCode::InvalidSeek;
            case ERROR_NOT_SUPPORTED:
                return IO::ResultCode::NotSupported;
            default:
                return IO::ResultCode::UnknownError;
            }
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
        case IO::StandardDescriptor::kSTDIN:
            stdDescriptor = STD_INPUT_HANDLE;
            break;
        case IO::StandardDescriptor::kSTDOUT:
            stdDescriptor = STD_OUTPUT_HANDLE;
            break;
        case IO::StandardDescriptor::kSTDERR:
            stdDescriptor = STD_ERROR_HANDLE;
            break;
        default:
            return FileHandle{};
        }

        return FileHandle::FromPointer(GetStdHandle(stdDescriptor));
    }


    IO::ResultCode OpenFile(const festd::string_view filePath, const IO::OpenMode openMode, FileHandle& handle)
    {
        FE_PROFILER_FUNCTION_TEXT("%.*s", filePath.size(), filePath.data());

        const WideString widePath{ filePath };
        if (widePath.m_value.empty())
            return IO::ResultCode::InvalidArgument;

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
            return ConvertWin32Error(GetLastError());

        handle = FileHandle::FromPointer(nativeFileHandle);
        return IO::ResultCode::Success;
    }


    IO::ResultCode GetFileStats(const FileHandle fileHandle, IO::FileStats& result)
    {
        FE_PROFILER_FUNCTION();

        const HANDLE hFile = HandleCast(fileHandle);
        FILETIME creationFT, accessFT, writeFT;
        if (!GetFileTime(hFile, &creationFT, &accessFT, &writeFT))
            return ConvertWin32Error(GetLastError());

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile, &fileSize))
            return ConvertWin32Error(GetLastError());

        result.m_creationTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(creationFT));
        result.m_accessTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(accessFT));
        result.m_modificationTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(writeFT));
        result.m_byteSize = static_cast<uint64_t>(fileSize.QuadPart);
        return IO::ResultCode::Success;
    }


    IO::FileAttributeFlags GetFileAttributeFlags(const festd::string_view filePath)
    {
        FE_PROFILER_FUNCTION_TEXT("%.*s", filePath.size(), filePath.data());

        const WideString widePath{ filePath };
        if (widePath.m_value.empty())
            return IO::FileAttributeFlags::kInvalid;

        const DWORD attributes = GetFileAttributesW(widePath.m_value.data());
        if (attributes == INVALID_FILE_ATTRIBUTES)
            return IO::FileAttributeFlags::kInvalid;

        IO::FileAttributeFlags result = IO::FileAttributeFlags::kNone;
        if (attributes & FILE_ATTRIBUTE_HIDDEN)
            result |= IO::FileAttributeFlags::kHidden;
        if (attributes & FILE_ATTRIBUTE_DIRECTORY)
            result |= IO::FileAttributeFlags::kDirectory;
        if (attributes & FILE_ATTRIBUTE_READONLY)
            result |= IO::FileAttributeFlags::kReadOnly;
        return result;
    }


    bool FileExists(const festd::string_view filePath)
    {
        FE_PROFILER_FUNCTION();
        const IO::FileAttributeFlags attributes = GetFileAttributeFlags(filePath);
        return attributes != IO::FileAttributeFlags::kInvalid && attributes != IO::FileAttributeFlags::kDirectory;
    }


    void CloseFile(const FileHandle fileHandle)
    {
        FE_PROFILER_FUNCTION();
        CloseHandle(HandleCast(fileHandle));
    }


    IO::ResultCode ReadFile(const FileHandle fileHandle, const festd::span<std::byte> buffer, uint32_t& bytesRead)
    {
        FE_PROFILER_FUNCTION_TEXT("%d", buffer.size());

        static_assert(sizeof(bytesRead) == sizeof(DWORD));

        if (!::ReadFile(HandleCast(fileHandle),
                        buffer.data(),
                        static_cast<DWORD>(buffer.size()),
                        reinterpret_cast<DWORD*>(&bytesRead),
                        nullptr))
        {
            return ConvertWin32Error(GetLastError());
        }

        return IO::ResultCode::Success;
    }


    IO::ResultCode WriteFile(const FileHandle fileHandle, const festd::span<const std::byte> buffer, uint32_t& bytesWritten)
    {
        FE_PROFILER_FUNCTION_TEXT("%d", buffer.size());

        static_assert(sizeof(bytesWritten) == sizeof(DWORD));

        if (!::WriteFile(HandleCast(fileHandle),
                         buffer.data(),
                         static_cast<DWORD>(buffer.size()),
                         reinterpret_cast<DWORD*>(&bytesWritten),
                         nullptr))
        {
            return ConvertWin32Error(GetLastError());
        }

        return IO::ResultCode::Success;
    }


    IO::ResultCode SeekFile(const FileHandle fileHandle, const intptr_t offset, const IO::SeekMode seekMode)
    {
        FE_PROFILER_FUNCTION();

        LARGE_INTEGER distance;
        distance.QuadPart = offset;
        if (!SetFilePointerEx(HandleCast(fileHandle), distance, nullptr, GetFileSeekMode(seekMode)))
        {
            return ConvertWin32Error(GetLastError());
        }

        return IO::ResultCode::Success;
    }


    IO::ResultCode TellFile(const FileHandle fileHandle, uintptr_t& position)
    {
        FE_PROFILER_FUNCTION();

        LARGE_INTEGER distance;
        distance.QuadPart = 0;
        if (!SetFilePointerEx(HandleCast(fileHandle), distance, reinterpret_cast<PLARGE_INTEGER>(&position), FILE_CURRENT))
        {
            return ConvertWin32Error(GetLastError());
        }

        return IO::ResultCode::Success;
    }
} // namespace FE::Platform
