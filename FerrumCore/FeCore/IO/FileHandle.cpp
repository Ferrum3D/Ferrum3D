#include <FeCore/Console/FeLog.h>
#include <FeCore/IO/FileHandle.h>

#if FE_WINDOWS
#    include <direct.h>
#    define FE_SEEK_64 _fseeki64
#    define FE_TELL_64 _ftelli64
#else
#    define FE_SEEK_64 fseek
#    define FE_SEEK_64 ftell
#endif

namespace FE::IO
{
    FileHandle::FileHandle()
        : m_Handle(nullptr)
    {
    }

    FileHandle::~FileHandle()
    {
        Close();
    }

    ResultCode FileHandle::Open(StringSlice fileName, OpenMode openMode)
    {
        bool shouldExist;
        switch (openMode)
        {
        case OpenMode::ReadOnly:
        case OpenMode::WriteOnly:
        case OpenMode::Append:
        case OpenMode::CreateNew:
        case OpenMode::ReadWrite:
            shouldExist = true;
            break;
        default:
            shouldExist = false;
            break;
        }

        if (shouldExist && !File::Exists(fileName))
        {
            return ResultCode::FileExists;
        }

        Close();

        GenFileOpenMode(openMode);
        m_Handle = fopen(fileName.Data(), m_OpenModeString);
        if (m_Handle)
        {
            m_FileName = fileName;
            m_OpenMode = openMode;
            return ResultCode::Success;
        }

        return Internal::HandleError(errno);
    }

    void FileHandle::Close()
    {
        if (IsOpen())
        {
            fclose(m_Handle);
            m_Handle   = nullptr;
            m_OpenMode = OpenMode::None;
            m_FileName.Clear();
        }
    }

    void FileHandle::GenFileOpenMode(OpenMode openMode)
    {
        SSize index = 0;
        switch (openMode)
        {
        case OpenMode::ReadOnly:
            m_OpenModeString[index++] = 'r';
            break;
        case OpenMode::Append:
            m_OpenModeString[index++] = 'a';
            break;
        case OpenMode::WriteOnly:
        case OpenMode::Create:
        case OpenMode::CreateNew:
            m_OpenModeString[index++] = 'w';
            break;
        case OpenMode::ReadWrite:
            m_OpenModeString[index++] = 'r';
            m_OpenModeString[index++] = '+';
            break;
        case OpenMode::Truncate:
            m_OpenModeString[index++] = 'w';
            m_OpenModeString[index++] = '+';
            break;
        default:
            FE_UNREACHABLE("Invalid FileOpenMode");
        }

        m_OpenModeString[index] = 'b';
    }

    ResultCode FileHandle::Seek(SSize offset, SeekMode seekMode)
    {
        FE_ASSERT_MSG(IsOpen(), "File not open");
        int origin = 0;
        switch (seekMode)
        {
        case SeekMode::Begin:
            origin = SEEK_SET;
            break;
        case SeekMode::End:
            origin = SEEK_END;
            break;
        case SeekMode::Current:
            origin = SEEK_CUR;
            break;
        }

        return FE_SEEK_64(m_Handle, offset, origin) ? ResultCode::InvalidSeek : ResultCode::Success;
    }

    USize FileHandle::Tell() const
    {
        FE_ASSERT_MSG(IsOpen(), "File not open");
        return FE_TELL_64(m_Handle);
    }

    USize FileHandle::Read(void* buffer, USize size)
    {
        FE_ASSERT_MSG(IsOpen(), "File not open");
        auto result = fread(buffer, 1, size, m_Handle);
        if (result == 0 && ferror(m_Handle))
        {
            FE_IO_ASSERT(Internal::HandleError(errno));
        }

        return result;
    }

    USize FileHandle::Write(const void* buffer, USize size)
    {
        FE_ASSERT_MSG(IsOpen(), "File not open");
        auto result = fwrite(buffer, 1, size, m_Handle);
        if (result == 0 && ferror(m_Handle))
        {
            FE_IO_ASSERT(Internal::HandleError(errno));
        }

        return result;
    }

    void FileHandle::Flush()
    {
        FE_ASSERT_MSG(IsOpen(), "File not open");
        fflush(m_Handle);
    }

    USize FileHandle::Length() const
    {
        FE_ASSERT_MSG(IsOpen(), "File not open");
        struct stat st; // NOLINT
        if (stat(m_FileName.Data(), &st) == 0)
        {
            return st.st_size;
        }

        Internal::HandleError(errno);
        return static_cast<USize>(-1);
    }

    DateTime FileHandle::GetLastModificationTime() const
    {
        FE_ASSERT_MSG(IsOpen(), "File not open");
        struct stat st; // NOLINT
        if (stat(m_FileName.Data(), &st) == 0)
        {
            return DateTime::CreateLocal(st.st_mtime);
        }

        Internal::HandleError(errno);
        return DateTime::Now();
    }

    StringSlice FileHandle::GetName() const
    {
        FE_ASSERT_MSG(IsOpen(), "File not open");
        return m_FileName;
    }

    bool FileHandle::IsOpen() const
    {
        return m_Handle;
    }

    OpenMode FileHandle::GetOpenMode() const
    {
        return m_OpenMode;
    }

    String Directory::GetCurrentDirectory()
    {
        static char buffer[256];
#if FE_WINDOWS
        _getcwd(buffer, sizeof(buffer));
#else
#    error Unsopported platform
#endif
        return buffer;
    }

    StringSlice Directory::GetParent(StringSlice fileName)
    {
        auto endIter = fileName.FindLastOf(FE_PATH_SEPARATOR);
        if (endIter == fileName.end())
        {
            endIter = fileName.FindLastOf('/');
        }

        return { fileName.begin(), endIter };
    }

    bool File::Exists(StringSlice fileName)
    {
        struct stat buffer; // NOLINT
        return stat(fileName.Data(), &buffer) == 0;
    }

    ResultCode File::Delete(StringSlice fileName)
    {
        if (remove(fileName.Data()) == 0)
        {
            return ResultCode::Success;
        }

        return Internal::HandleError(errno);
    }

    String File::ReadAllText(StringSlice fileName)
    {
        FileHandle file;
        auto result = file.Open(fileName, OpenMode::ReadOnly);
        FE_ASSERT_MSG(
            result == ResultCode::Success, "IO Error while loading file {}: {}", Directory::GetCurrentDirectory() / fileName,
            GetResultDesc(result));

        auto length = file.Length();
        String buffer(length, ' ');
        file.Read(buffer.Data(), length);
        return buffer;
    }
} // namespace FE::IO
