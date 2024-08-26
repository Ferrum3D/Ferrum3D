#pragma once
#include <FeCore/Strings/StringSlice.h>

namespace FE::IO
{
    //! \brief Represents an I/O result code.
    enum class ResultCode : int32_t
    {
        Success = 0,
        PermissionDenied = -1,  //!< Permission denied.
        NoFileOrDirectory = -2, //!< No such file or directory.
        FileExists = -3,        //!< File already exists.
        FileTooLarge = -4,      //!< File is too large.
        FilenameTooLong = -5,   //!< Filename is too long.
        NotDirectory = -6,      //!< Not a directory.
        IsDirectory = -7,       //!< Is a directory.
        DirectoryNotEmpty = -8, //!< Directory is not empty.
        TooManyOpenFiles = -9,  //!< Too many files are open.
        InvalidSeek = -10,      //!< Invalid seek operation.
        IOError = -11,          //!< IO error.
        DeadLock = -12,         //!< Resource deadlock would occur.
        UnknownError = DefaultErrorCode<ResultCode>,
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
} // namespace FE::IO
