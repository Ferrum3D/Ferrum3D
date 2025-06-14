#include <FeCore/IO/BaseIO.h>
#include <FeCore/IO/Platform/PlatformPath.h>

namespace FE::IO
{
    festd::string_view GetResultDesc(const ResultCode code)
    {
        switch (code)
        {
        case ResultCode::kSuccess:
            return "Success";
        case ResultCode::kCanceled:
            return "Operation was canceled";
        case ResultCode::kPermissionDenied:
            return "Permission denied";
        case ResultCode::kNoFileOrDirectory:
            return "No such file or directory";
        case ResultCode::kFileExists:
            return "File already exists";
        case ResultCode::kFileTooLarge:
            return "File is too large";
        case ResultCode::kFilenameTooLong:
            return "Filename is too long";
        case ResultCode::kNotDirectory:
            return "Not a directory";
        case ResultCode::kIsDirectory:
            return "Is a directory";
        case ResultCode::kDirectoryNotEmpty:
            return "Directory is not empty";
        case ResultCode::kTooManyOpenFiles:
            return "Too many files are open";
        case ResultCode::kInvalidSeek:
            return "Invalid seek operation";
        case ResultCode::kIOError:
            return "IO error";
        case ResultCode::kDeadLock:
            return "Resource deadlock would occur";
        case ResultCode::kNotSupported:
            return "Operation is not supported";
        case ResultCode::kInvalidArgument:
            return "Argument value has not been accepted";
        case ResultCode::kInvalidFormat:
            return "Invalid file format";
        case ResultCode::kDecompressionError:
            return "Block file decompression failed";
        case ResultCode::kUnknownError:
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
