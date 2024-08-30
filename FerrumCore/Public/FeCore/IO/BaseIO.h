#pragma once
#include <FeCore/Strings/FixedString.h>
#include <FeCore/Strings/StringSlice.h>
#include <FeCore/Time/DateTime.h>

namespace FE::IO
{
    inline constexpr uint32_t MaxPathLength = 260;

    using FixedPath = FixedString<MaxPathLength>;


    //! @brief Represents an I/O result code.
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
        NotSupported = -13,     //!< Operation is not supported.
        InvalidArgument = -14,  //!< Argument value has not been accepted.
        UnknownError = DefaultErrorCode<ResultCode>,
    };

    StringSlice GetResultDesc(ResultCode code);

#define FE_IO_ASSERT(expr)                                                                                                       \
    do                                                                                                                           \
    {                                                                                                                            \
        ::FE::IO::ResultCode code = expr;                                                                                        \
        FE_AssertMsg(code == ::FE::IO::ResultCode::Success, "IO error: {}", ::FE::IO::GetResultDesc(code));                      \
    }                                                                                                                            \
    while (0)


    namespace Platform
    {
        struct FileHandle final : TypedHandle<FileHandle, uint64_t>
        {
            inline static FileHandle FromPointer(const void* ptr)
            {
                return FileHandle{ reinterpret_cast<uint64_t>(ptr) };
            }
        };
    } // namespace Platform


    FixedPath GetCurrentDirectory();


    //! @brief I/O operation priority.
    //!
    //! To make a more fine-grained request priority class
    //! use addition and subtraction, e.g. IO::Priority::kNormal + 123
    //! would be somewhere between normal and high priorities.
    //!
    //! The difference between two consecutive priority levels is Priority::kLevelDiff.
    enum class Priority : int32_t
    {
        kLevelBits = 20,
        kLevelDiff = (1 << kLevelBits) - 1,

        kLowest = 1 << kLevelBits,
        kLow = 2 << kLevelBits,
        kNormal = 3 << kLevelBits,
        kHigh = 4 << kLevelBits,
        kHighest = 5 << kLevelBits,
    };


    inline Priority operator-(Priority lhs, int32_t rhs)
    {
        return static_cast<Priority>(enum_cast(lhs) - rhs);
    }


    inline Priority operator+(Priority lhs, int32_t rhs)
    {
        return static_cast<Priority>(enum_cast(lhs) + rhs);
    }


    enum class StandardDescriptor
    {
        kSTDIN,
        kSTDOUT,
        kSTDERR,
    };


    struct FileStats final
    {
        DateTime<TZ::UTC> CreationTime;
        DateTime<TZ::UTC> ModificationTime;
        DateTime<TZ::UTC> AccessTime;
        uint64_t ByteSize;
    };


    enum class FileAttributeFlags : uint32_t
    {
        kNone = 0,
        kHidden = 1 << 0,
        kDirectory = 1 << 1,
        kReadOnly = 1 << 2,
        kInvalid = 1u << 31,
    };

    FE_ENUM_OPERATORS(FileAttributeFlags);


    enum class OpenMode
    {
        kNone,
        kReadOnly = 1 << 6,
        kWriteOnly = 1 << 7,
        kAppend = kWriteOnly | 1,
        kCreate = kWriteOnly | 2,
        kCreateNew = kWriteOnly | 3,
        kTruncate = kWriteOnly | 4,
        kReadWrite = kReadOnly | kWriteOnly,
    };


    inline constexpr bool IsWriteAllowed(OpenMode mode)
    {
        return (enum_cast(mode) & enum_cast(OpenMode::kWriteOnly)) != 0;
    }


    inline constexpr bool IsReadAllowed(OpenMode mode)
    {
        return (enum_cast(mode) & enum_cast(OpenMode::kReadOnly)) != 0;
    }


    enum class SeekMode
    {
        kBegin,
        kEnd,
        kCurrent
    };


    inline constexpr const char* GetStandardDescriptorName(StandardDescriptor descriptor)
    {
        switch (descriptor)
        {
        case StandardDescriptor::kSTDIN:
            return "STDIN";
        case StandardDescriptor::kSTDOUT:
            return "STDOUT";
        case StandardDescriptor::kSTDERR:
            return "STDERR";
        default:
            return "<Unknown>";
        }
    }


    inline constexpr OpenMode GetStandardDescriptorOpenMode(StandardDescriptor descriptor)
    {
        switch (descriptor)
        {
        case StandardDescriptor::kSTDIN:
            return OpenMode::kReadOnly;
        case StandardDescriptor::kSTDOUT:
            return OpenMode::kAppend;
        case StandardDescriptor::kSTDERR:
            return OpenMode::kAppend;
        default:
            return OpenMode::kNone;
        }
    }
} // namespace FE::IO
