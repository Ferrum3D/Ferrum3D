#pragma once
#include <FeCore/IO/Path.h>
#include <FeCore/RTTI/RTTI.h>
#include <FeCore/Time/DateTime.h>

namespace FE::Platform
{
    struct FileHandle final : public TypedHandle<FileHandle, uint64_t>
    {
        static FileHandle FromPointer(const void* ptr)
        {
            return FileHandle{ reinterpret_cast<uint64_t>(ptr) };
        }
    };
} // namespace FE::Platform


namespace FE::IO
{
    struct IStream;
    struct IStreamFactory;

    struct IAsyncController;
    struct IAsyncStreamIO;
    struct AsyncReadResult;


    //! @brief Represents an I/O result code.
    enum class ResultCode : int32_t
    {
        Success = 0,
        Canceled = 1,           //!< Operation was canceled.
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
        UnknownError = kDefaultErrorCode<ResultCode>,
    };

    festd::string_view GetResultDesc(ResultCode code);

#define FE_IO_ASSERT(expr)                                                                                                       \
    do                                                                                                                           \
    {                                                                                                                            \
        ::FE::IO::ResultCode code = expr;                                                                                        \
        FE_AssertMsg(code == ::FE::IO::ResultCode::Success, "IO error: {}", ::FE::IO::GetResultDesc(code));                      \
    }                                                                                                                            \
    while (0)


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


    inline Priority operator-(const Priority lhs, const int32_t rhs)
    {
        return static_cast<Priority>(festd::to_underlying(lhs) - rhs);
    }


    inline Priority operator+(const Priority lhs, const int32_t rhs)
    {
        return static_cast<Priority>(festd::to_underlying(lhs) + rhs);
    }


    enum class StandardDescriptor
    {
        kSTDIN,
        kSTDOUT,
        kSTDERR,
    };


    struct FileStats final
    {
        DateTime<TZ::UTC> m_creationTime;
        DateTime<TZ::UTC> m_modificationTime;
        DateTime<TZ::UTC> m_accessTime;
        uint64_t m_byteSize;
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
        kNone = 0,
        kReadOnly = 1 << 6,
        kWriteOnly = 1 << 7,
        kAppend = kWriteOnly | 1,
        kCreate = kWriteOnly | 2,
        kCreateNew = kWriteOnly | 3,
        kTruncate = kWriteOnly | 4,
        kReadWrite = kReadOnly | kWriteOnly,
    };


    struct DirectoryEntry final
    {
        PathView m_path;
        FileAttributeFlags m_attributes = FileAttributeFlags::kNone;
        FileStats m_stats;
    };


    constexpr bool IsWriteAllowed(const OpenMode mode)
    {
        return (festd::to_underlying(mode) & festd::to_underlying(OpenMode::kWriteOnly)) != 0;
    }


    constexpr bool IsReadAllowed(const OpenMode mode)
    {
        return (festd::to_underlying(mode) & festd::to_underlying(OpenMode::kReadOnly)) != 0;
    }


    enum class SeekMode
    {
        kBegin,
        kEnd,
        kCurrent
    };


    constexpr const char* GetStandardDescriptorName(const StandardDescriptor descriptor)
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


    constexpr OpenMode GetStandardDescriptorOpenMode(const StandardDescriptor descriptor)
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


    //! @brief Asynchronous read operation callback.
    struct IAsyncReadCallback
    {
        FE_RTTI_Class(IAsyncReadCallback, "E1E0BD22-543A-4036-B918-134DB9C99D4F");

        virtual ~IAsyncReadCallback() = default;

        //! @brief Called when an operation associated with this callback completes.
        virtual void AsyncIOCallback(const AsyncReadResult& result) = 0;
    };


    namespace Directory
    {
        //! @brief Iterate over a directory recursively.
        //!
        //! @param path    Path to the directory to iterate over.
        //! @param pattern Wildcard pattern.
        //! @param f       The function to be called for each directory entry.
        ResultCode TraverseRecursively(festd::string_view path, festd::string_view pattern,
                                       const festd::fixed_function<48, bool(const DirectoryEntry&)>& f);


        //! @brief Iterate over a directory recursively.
        //!
        //! @param path    Path to the directory to iterate over.
        //! @param f       The function to be called for each directory entry.
        inline ResultCode TraverseRecursively(const festd::string_view path,
                                              const festd::fixed_function<48, bool(const DirectoryEntry&)>& f)
        {
            return TraverseRecursively(path, "*", f);
        }
    } // namespace Directory
} // namespace FE::IO
