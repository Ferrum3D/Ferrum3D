#pragma once
#include <FeCore/Strings/StringSlice.h>

namespace FE::IO
{
    enum class ResultCode
    {
        Success,
        PermissionDenied,
        NoFileOrDirectory,
        FileExists,
        FileTooLarge,
        FilenameTooLong,
        NotDirectory,
        IsDirectory,
        DirectoryNotEmpty,
        TooManyOpenFiles,
        InvalidSeek,
        IOError,
        DeadLock,
        UnknownError
    };

    StringSlice GetResultDesc(ResultCode code);

#define FE_IO_ASSERT(expr)                                                                                                       \
    do                                                                                                                           \
    {                                                                                                                            \
        ::FE::IO::ResultCode code = expr;                                                                                        \
        FE_ASSERT_MSG(code == ::FE::IO::ResultCode::Success, "IO error: {}", ::FE::IO::GetResultDesc(code));                     \
    }                                                                                                                            \
    while (0)

    enum class OpenMode
    {
        None,
        ReadOnly,
        WriteOnly,
        Append,
        Create,
        CreateNew,
        Truncate,
        ReadWrite
    };

    inline bool IsWriteAllowed(OpenMode mode)
    {
        switch (mode)
        {
        case OpenMode::WriteOnly:
        case OpenMode::Append:
        case OpenMode::Create:
        case OpenMode::CreateNew:
        case OpenMode::Truncate:
        case OpenMode::ReadWrite:
            return true;
        default:
            return false;
        }
    }

    inline bool IsReadAllowed(OpenMode mode)
    {
        switch (mode)
        {
        case OpenMode::ReadOnly:
        case OpenMode::ReadWrite:
            return true;
        default:
            return false;
        }
    }

    enum class SeekMode
    {
        Begin,
        End,
        Current
    };
}
