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
    struct AsyncBlockReadResult;


    //! @brief Represents an I/O result code.
    enum class ResultCode : int32_t
    {
        kSuccess = 0,
        kCanceled = 1,             //!< Operation was canceled.
        kPermissionDenied = -1,    //!< Permission denied.
        kNoFileOrDirectory = -2,   //!< No such file or directory.
        kFileExists = -3,          //!< File already exists.
        kFileTooLarge = -4,        //!< File is too large.
        kFilenameTooLong = -5,     //!< Filename is too long.
        kNotDirectory = -6,        //!< Not a directory.
        kIsDirectory = -7,         //!< Is a directory.
        kDirectoryNotEmpty = -8,   //!< Directory is not empty.
        kTooManyOpenFiles = -9,    //!< Too many files are open.
        kInvalidSeek = -10,        //!< Invalid seek operation.
        kIOError = -11,            //!< IO error.
        kDeadLock = -12,           //!< Resource deadlock would occur.
        kNotSupported = -13,       //!< Operation is not supported.
        kInvalidArgument = -14,    //!< Argument value has not been accepted.
        kInvalidFormat = -15,      //!< Invalid file format.
        kDecompressionError = -16, //!< Block file decompression failed.
        kUnknownError = kDefaultErrorCode<ResultCode>,
    };

    festd::string_view GetResultDesc(ResultCode code);

#define FE_IO_ASSERT(expr)                                                                                                       \
    do                                                                                                                           \
    {                                                                                                                            \
        ::FE::IO::ResultCode code = expr;                                                                                        \
        FE_AssertMsg(code == ::FE::IO::ResultCode::kSuccess, "IO error: {}", ::FE::IO::GetResultDesc(code));                     \
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
        kLevelDiff = 1 << kLevelBits,

        kLowest = 0,
        kLow = 1 << kLevelBits,
        kNormal = 2 << kLevelBits,
        kHigh = 3 << kLevelBits,
        kHighest = 4 << kLevelBits,
    };


    constexpr Priority operator-(const Priority lhs, const int32_t rhs)
    {
        return static_cast<Priority>(festd::to_underlying(lhs) - rhs);
    }


    constexpr Priority operator+(const Priority lhs, const int32_t rhs)
    {
        return static_cast<Priority>(festd::to_underlying(lhs) + rhs);
    }


    enum class StandardDescriptor
    {
        kStdin,
        kStdout,
        kStderr,
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
        kHidden = 1u << 0,
        kDirectory = 1u << 1,
        kReadOnly = 1u << 2,
        kInvalid = 1u << 31,
    };

    FE_ENUM_OPERATORS(FileAttributeFlags);


    //! @brief File open mode.
    //!
    //! The modes which allow writing have the kWriteOnly flag set.
    //! The modes which allow reading have the kReadOnly flag set.
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


    inline const char* GetStandardDescriptorName(const StandardDescriptor descriptor)
    {
        switch (descriptor)
        {
        default:
            FE_DebugBreak();
            return "<Unknown>";

        case StandardDescriptor::kStdin:
            return "STDIN";
        case StandardDescriptor::kStdout:
            return "STDOUT";
        case StandardDescriptor::kStderr:
            return "STDERR";
        }
    }


    inline OpenMode GetStandardDescriptorOpenMode(const StandardDescriptor descriptor)
    {
        switch (descriptor)
        {
        default:
            FE_DebugBreak();
            return OpenMode::kNone;

        case StandardDescriptor::kStdin:
            return OpenMode::kReadOnly;
        case StandardDescriptor::kStdout:
        case StandardDescriptor::kStderr:
            return OpenMode::kAppend;
        }
    }


    //! @brief Asynchronous read operation callback.
    struct IAsyncReadCallback
    {
        FE_RTTI_Class(IAsyncReadCallback, "E1E0BD22-543A-4036-B918-134DB9C99D4F");

        virtual ~IAsyncReadCallback() = default;

        //! @brief Called when an operation associated with this callback completes.
        virtual void AsyncIOCallback([[maybe_unused]] const AsyncReadResult& result) {}

        //! @brief Called when an operation associated with this callback completes.
        virtual void AsyncIOCallback([[maybe_unused]] const AsyncBlockReadResult& result) {}
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
