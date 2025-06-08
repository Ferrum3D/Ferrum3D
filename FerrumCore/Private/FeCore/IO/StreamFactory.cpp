#include <FeCore/IO/FileStream.h>
#include <FeCore/IO/Platform/PlatformFile.h>
#include <FeCore/IO/StreamFactory.h>

namespace FE::IO
{
    FileStreamFactory::FileStreamFactory(Env::Configuration* pConfig)
        : m_fileStreamPool("IO/FileStream", sizeof(FileStream), 64 * 1024)
    {
        const Path currentDirectory = Directory::GetCurrentDirectory();
        m_parentDirectory = pConfig->GetString("AssetDirectory", currentDirectory);
        Directory::SetCurrentDirectory(m_parentDirectory);
    }


    festd::expected<Rc<IStream>, ResultCode> FileStreamFactory::OpenFileStream(const festd::string_view filename,
                                                                               const OpenMode openMode)
    {
        FE_PROFILER_ZONE();

        const Rc fileStream = Rc<FileStream>::New(&m_fileStreamPool);
        const Path fullPath = m_parentDirectory / filename;
        const ResultCode result = fileStream->Open(fullPath, openMode);
        if (result != ResultCode::kSuccess)
            return festd::unexpected(result);

        return static_pointer_cast<IStream>(fileStream);
    }


    festd::expected<Rc<IStream>, ResultCode> FileStreamFactory::OpenUnbufferedFileStream(festd::string_view filename,
                                                                                         OpenMode openMode)
    {
        FE_PROFILER_ZONE();

        const Rc fileStream = Rc<FileStream>::New(&m_fileStreamPool);
        fileStream->SetBufferSize(0);

        const Path fullPath = m_parentDirectory / filename;
        const ResultCode result = fileStream->Open(fullPath, openMode);
        if (result != ResultCode::kSuccess)
            return festd::unexpected(result);

        return static_pointer_cast<IStream>(fileStream);
    }


    bool FileStreamFactory::FileExists(const festd::string_view filename)
    {
        const Path fullPath = m_parentDirectory / filename;
        return Platform::FileExists(fullPath);
    }


    FileAttributeFlags FileStreamFactory::GetFileAttributeFlags(const festd::string_view filename)
    {
        return Platform::GetFileAttributeFlags(filename);
    }
} // namespace FE::IO
