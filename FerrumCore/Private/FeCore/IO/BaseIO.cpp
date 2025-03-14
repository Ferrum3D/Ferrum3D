#include <FeCore/IO/BaseIO.h>
#include <FeCore/IO/Platform/PlatformPath.h>

namespace FE::IO
{
    festd::string_view GetResultDesc(const ResultCode code)
    {
        switch (code)
        {
        case ResultCode::Success:
            return "Success";
        case ResultCode::Canceled:
            return "Operation was canceled";
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


    ResultCode Directory::TraverseRecursively(const festd::string_view path, const festd::string_view pattern,
                                              const festd::fixed_function<48, bool(const DirectoryEntry&)>& f)
    {
        FE_PROFILER_ZONE_TEXT("%.*s", path.size(), path.data());

        Platform::DirectoryIterationParams params;
        params.m_path = GetAbsolutePath(path);
        params.m_pattern = pattern;
        params.m_callbackData = reinterpret_cast<uintptr_t>(&f);
        params.m_callback = [](const uintptr_t callbackData, const DirectoryEntry& entry) {
            return (*reinterpret_cast<const std::decay_t<decltype(f)>*>(callbackData))(entry);
        };

        return Platform::IterateDirectoryRecursively(params);
    }
} // namespace FE::IO
