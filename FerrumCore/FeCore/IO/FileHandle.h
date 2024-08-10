#pragma once
#include <FeCore/Strings/StringSlice.h>
#include <FeCore/Time/DateTime.h>
#include <FeCore/IO/BaseIO.h>
#include <FeCore/Console/FeLog.h>

namespace FE::IO
{
    namespace Internal
    {
        inline ResultCode GetResultCode(errno_t err)
        {
            switch (err)
            {
            case ENOENT:
                return ResultCode::NoFileOrDirectory;
            case EACCES:
                return ResultCode::PermissionDenied;
            case ENAMETOOLONG:
                return ResultCode::FilenameTooLong;
            case EIO:
                return ResultCode::IOError;
            case EEXIST:
                return ResultCode::FileExists;
            case ENOTDIR:
                return ResultCode::NotDirectory;
            case EISDIR:
                return ResultCode::IsDirectory;
            case EMFILE:
                return ResultCode::TooManyOpenFiles;
            case EFBIG:
                return ResultCode::FileTooLarge;
            case ENOTEMPTY:
                return ResultCode::DirectoryNotEmpty;
            case EDEADLK:
                return ResultCode::DeadLock;
            case ESPIPE:
                return ResultCode::InvalidSeek;
            default:
                return ResultCode::UnknownError;
            }
        }

        inline ResultCode HandleError(errno_t err)
        {
            auto code = GetResultCode(err);
            if (code != ResultCode::Success)
            {
                // Emit warning in debug builds. Not error, because it can be handled.
                // If application fails to handle the error, FE_IO_ASSERT will emit it.
                FE_LOG_WARNING("I/O error occurred: {}", GetResultDesc(code));
            }
            return code;
        }
    }

    struct Directory
    {
        static String GetCurrentDirectory();
        static StringSlice GetParent(StringSlice fileName);
    };

    struct File
    {
        static bool Exists(StringSlice fileName);
        static String ReadAllText(StringSlice fileName);
        static ResultCode Delete(StringSlice fileName);
    };


    class FileHandle final : public Memory::RefCountedObjectBase
    {
        FILE* m_Handle;
        String m_FileName{};

        char m_OpenModeString[8]{};
        OpenMode m_OpenMode = OpenMode::None;

        void GenFileOpenMode(OpenMode openMode);

    public:
        FE_RTTI_Class(FileHandle, "58D19D75-CE53-4B11-B151-F82583B3EAD8");

        FileHandle();
        ~FileHandle() override;

        ResultCode Open(StringSlice fileName, OpenMode openMode);
        void Close();

        ResultCode Seek(ptrdiff_t offset, SeekMode seekMode);

        [[nodiscard]] size_t Tell() const;

        [[nodiscard]] DateTime GetLastModificationTime() const;
        [[nodiscard]] OpenMode GetOpenMode() const;

        size_t Read(void* buffer, size_t size);
        size_t Write(const void* buffer, size_t size);

        void Flush();

        [[nodiscard]] size_t Length() const;

        [[nodiscard]] StringSlice GetName() const;

        [[nodiscard]] bool IsOpen() const;
    };
} // namespace FE::IO
