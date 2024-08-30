#include <FeCore/Base/PlatformInclude.h>
#include <FeCore/IO/BaseIO.h>

namespace FE::IO
{
    FixedPath GetCurrentDirectory()
    {
#if FE_WINDOWS
        wchar_t buf[MaxPathLength];
        GetCurrentDirectoryW(MaxPathLength, buf);

        const int32_t length = WideCharToMultiByte(CP_UTF8, 0, buf, -1, nullptr, 0, nullptr, nullptr);
        assert(length > 0);

        FixedPath result;
        result.Resize(length - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, buf, -1, result.Data(), result.Size(), nullptr, nullptr);
        return result;
#else
#    error Not implemented :(
#endif
    }


    StringSlice GetResultDesc(ResultCode code)
    {
        switch (code)
        {
        case ResultCode::Success:
            return "Success";
        case ResultCode::PermissionDenied:
            return "Permission denied";
        case ResultCode::NoFileOrDirectory:
            return "No such file or directory";
        case ResultCode::FileExists:
            return "File already exists";
        case ResultCode::FileTooLarge:
            return "File is too large";
        case ResultCode::FilenameTooLong:
            return "Filename is too long";
        case ResultCode::NotDirectory:
            return "Not a directory";
        case ResultCode::IsDirectory:
            return "Is a directory";
        case ResultCode::DirectoryNotEmpty:
            return "Directory is not empty";
        case ResultCode::TooManyOpenFiles:
            return "Too many files are open";
        case ResultCode::InvalidSeek:
            return "Invalid seek operation";
        case ResultCode::IOError:
            return "IO error";
        case ResultCode::DeadLock:
            return "Resource deadlock would occur";
        case ResultCode::NotSupported:
            return "Operation is not supported";
        case ResultCode::InvalidArgument:
            return "Argument value has not been accepted";
        default:
            return "Unknown error";
        }
    }
} // namespace FE::IO
