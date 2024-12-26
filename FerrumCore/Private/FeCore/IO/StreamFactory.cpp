#include <FeCore/IO/FileStream.h>
#include <FeCore/IO/Platform/PlatformFile.h>
#include <FeCore/IO/StreamFactory.h>

namespace FE::IO
{
    FileStreamFactory::FileStreamFactory(Env::Configuration* pConfig)
        : m_fileStreamPool("IO/FileStream", sizeof(FileStream), 64 * 1024)
    {
        const IO::FixedPath currentDirectory = GetCurrentDirectory();
        m_parentDirectory = pConfig->GetString("AssetDirectory", currentDirectory);
    }


    festd::expected<Rc<IStream>, ResultCode> FileStreamFactory::OpenFileStream(StringSlice filename, OpenMode openMode)
    {
        const Rc fileStream = Rc<FileStream>::New(&m_fileStreamPool);
        const FixedPath fullPath = m_parentDirectory / filename;
        const ResultCode result = fileStream->Open(fullPath, openMode);
        if (result != ResultCode::Success)
            return festd::unexpected(result);

        return static_pointer_cast<IStream>(fileStream);
    }


    bool FileStreamFactory::FileExists(StringSlice filename)
    {
        const FixedPath fullPath = m_parentDirectory / filename;
        return Platform::FileExists(fullPath);
    }


    FileAttributeFlags FileStreamFactory::GetFileAttributeFlags(StringSlice filename)
    {
        return Platform::GetFileAttributeFlags(filename);
    }
} // namespace FE::IO
