#include <FeCore/Containers/SmallVector.h>
#include <FeCore/IO/Platform/PlatformFile.h>
#include <FeCore/Platform/Windows/Common.h>

namespace FE::IO::Platform
{
    inline static HANDLE HandleCast(FileHandle handle)
    {
        return reinterpret_cast<HANDLE>(handle.m_value);
    }


    inline static ResultCode ConvertWin32Error(DWORD error)
    {
        switch (error)
        {
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            return ResultCode::FileExists;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return ResultCode::NoFileOrDirectory;
        case ERROR_ACCESS_DENIED:
            return ResultCode::PermissionDenied;
        case ERROR_SHARING_VIOLATION:
        case ERROR_INVALID_PARAMETER:
            return ResultCode::InvalidArgument;
        case ERROR_FILE_TOO_LARGE:
            return ResultCode::FileTooLarge;
        case ERROR_TOO_MANY_OPEN_FILES:
            return ResultCode::TooManyOpenFiles;
        case ERROR_SEEK:
            return ResultCode::InvalidSeek;
        case ERROR_NOT_SUPPORTED:
            return ResultCode::NotSupported;
        default:
            return ResultCode::UnknownError;
        }
    }


    inline static DWORD GetFileAccessFlags(OpenMode openMode)
    {
        switch (openMode)
        {
        case OpenMode::kNone:
            return 0;
        case OpenMode::kReadOnly:
            return GENERIC_READ;
        case OpenMode::kWriteOnly:
        case OpenMode::kCreate:
        case OpenMode::kCreateNew:
            return GENERIC_WRITE;
        case OpenMode::kAppend:
        case OpenMode::kTruncate:
        case OpenMode::kReadWrite:
            return GENERIC_READ | GENERIC_WRITE;
        default:
            return 0;
        }
    }


    inline static DWORD GetFileShareMode(OpenMode openMode)
    {
        if (openMode == OpenMode::kReadOnly)
            return FILE_SHARE_READ;

        return 0;
    }


    inline static DWORD GetFileCreationDisposition(OpenMode openMode)
    {
        switch (openMode)
        {
        case OpenMode::kNone:
            return 0;
        case OpenMode::kReadOnly:
        case OpenMode::kWriteOnly:
        case OpenMode::kReadWrite:
        case OpenMode::kAppend:
            return OPEN_EXISTING;
        case OpenMode::kCreate:
            return CREATE_ALWAYS;
        case OpenMode::kCreateNew:
            return CREATE_NEW;
        case OpenMode::kTruncate:
            return TRUNCATE_EXISTING;
        default:
            return 0;
        }
    }


    inline static DWORD GetFileSeekMode(SeekMode seekMode)
    {
        switch (seekMode)
        {
        case SeekMode::kBegin:
            return FILE_BEGIN;
        case SeekMode::kCurrent:
            return FILE_CURRENT;
        case SeekMode::kEnd:
            return FILE_END;
        default:
            return 0;
        }
    }


    FileHandle GetStandardFile(StandardDescriptor descriptor)
    {
        DWORD stdDescriptor;
        switch (descriptor)
        {
        case StandardDescriptor::kSTDIN:
            stdDescriptor = STD_INPUT_HANDLE;
            break;
        case StandardDescriptor::kSTDOUT:
            stdDescriptor = STD_OUTPUT_HANDLE;
            break;
        case StandardDescriptor::kSTDERR:
            stdDescriptor = STD_ERROR_HANDLE;
            break;
        default:
            return FileHandle{};
        }

        return FileHandle::FromPointer(GetStdHandle(stdDescriptor));
    }


    ResultCode OpenFile(StringSlice filePath, OpenMode openMode, FileHandle& handle)
    {
        ZoneScoped;
        ZoneTextF("%.*s", filePath.Size(), filePath.Data());

        using namespace FE::Platform;

        const WideString<MAX_PATH> widePath{ filePath };
        if (widePath.Value.empty())
            return ResultCode::InvalidArgument;

        const DWORD desiredFileAccessFlags = GetFileAccessFlags(openMode);
        const DWORD fileShareMode = GetFileShareMode(openMode);
        const DWORD fileCreationDisposition = GetFileCreationDisposition(openMode);

        const HANDLE nativeFileHandle = CreateFileW(widePath.Value.data(),
                                                    desiredFileAccessFlags,
                                                    fileShareMode,
                                                    nullptr,
                                                    fileCreationDisposition,
                                                    FILE_ATTRIBUTE_NORMAL,
                                                    nullptr);

        if (nativeFileHandle == INVALID_HANDLE_VALUE)
            return ConvertWin32Error(GetLastError());

        handle = FileHandle::FromPointer(nativeFileHandle);
        return ResultCode::Success;
    }


    ResultCode GetFileStats(FileHandle fileHandle, FileStats& result)
    {
        ZoneScoped;

        using namespace FE::Platform;

        const HANDLE hFile = HandleCast(fileHandle);
        FILETIME creationFT, accessFT, writeFT;
        if (!GetFileTime(hFile, &creationFT, &accessFT, &writeFT))
            return ConvertWin32Error(GetLastError());

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile, &fileSize))
            return ConvertWin32Error(GetLastError());

        result.CreationTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(creationFT));
        result.AccessTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(accessFT));
        result.ModificationTime = DateTime<TZ::UTC>::FromUnixTime(ConvertFiletimeToUnixSeconds(writeFT));
        result.ByteSize = static_cast<uint64_t>(fileSize.QuadPart);
        return ResultCode::Success;
    }


    FileAttributeFlags GetFileAttributeFlags(StringSlice filePath)
    {
        ZoneScoped;
        ZoneTextF("%.*s", filePath.Size(), filePath.Data());

        using namespace FE::Platform;

        const WideString<MAX_PATH> widePath{ filePath };
        if (widePath.Value.empty())
            return FileAttributeFlags::kInvalid;

        const DWORD attributes = GetFileAttributesW(widePath.Value.data());
        if (attributes == INVALID_FILE_ATTRIBUTES)
            return FileAttributeFlags::kInvalid;

        FileAttributeFlags result = FileAttributeFlags::kNone;
        if (attributes & FILE_ATTRIBUTE_HIDDEN)
            result |= FileAttributeFlags::kHidden;
        if (attributes & FILE_ATTRIBUTE_DIRECTORY)
            result |= FileAttributeFlags::kDirectory;
        if (attributes & FILE_ATTRIBUTE_READONLY)
            result |= FileAttributeFlags::kReadOnly;
        return result;
    }


    bool FileExists(StringSlice filePath)
    {
        ZoneScoped;
        const FileAttributeFlags attributes = GetFileAttributeFlags(filePath);
        return attributes != FileAttributeFlags::kInvalid && attributes != FileAttributeFlags::kDirectory;
    }


    void CloseFile(FileHandle fileHandle)
    {
        ZoneScoped;
        CloseHandle(HandleCast(fileHandle));
    }


    ResultCode ReadFile(FileHandle fileHandle, festd::span<std::byte> buffer, uint32_t& bytesRead)
    {
        ZoneScoped;
        ZoneTextF("%d", buffer.size());

        static_assert(sizeof(bytesRead) == sizeof(DWORD));

        if (!::ReadFile(HandleCast(fileHandle),
                        buffer.data(),
                        static_cast<DWORD>(buffer.size()),
                        reinterpret_cast<DWORD*>(&bytesRead),
                        nullptr))
        {
            return ConvertWin32Error(GetLastError());
        }

        return ResultCode::Success;
    }


    ResultCode WriteFile(FileHandle fileHandle, festd::span<const std::byte> buffer, uint32_t& bytesWritten)
    {
        ZoneScoped;
        ZoneTextF("%d", buffer.size());

        static_assert(sizeof(bytesWritten) == sizeof(DWORD));

        if (!::WriteFile(HandleCast(fileHandle),
                         buffer.data(),
                         static_cast<DWORD>(buffer.size()),
                         reinterpret_cast<DWORD*>(&bytesWritten),
                         nullptr))
        {
            return ConvertWin32Error(GetLastError());
        }

        return ResultCode::Success;
    }


    ResultCode SeekFile(FileHandle fileHandle, intptr_t offset, SeekMode seekMode)
    {
        ZoneScoped;

        LARGE_INTEGER distance;
        distance.QuadPart = offset;
        if (!SetFilePointerEx(HandleCast(fileHandle), distance, nullptr, GetFileSeekMode(seekMode)))
        {
            return ConvertWin32Error(GetLastError());
        }

        return ResultCode::Success;
    }


    ResultCode TellFile(FileHandle fileHandle, uintptr_t& position)
    {
        ZoneScoped;

        LARGE_INTEGER distance;
        distance.QuadPart = 0;
        if (!SetFilePointerEx(HandleCast(fileHandle), distance, reinterpret_cast<PLARGE_INTEGER>(&position), FILE_CURRENT))
        {
            return ConvertWin32Error(GetLastError());
        }

        return ResultCode::Success;
    }
} // namespace FE::IO::Platform
