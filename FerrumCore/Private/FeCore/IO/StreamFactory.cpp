#include <FeCore/IO/FileStream.h>
#include <FeCore/IO/Platform/PlatformFile.h>
#include <FeCore/IO/StreamFactory.h>

namespace FE::IO
{
    FileStreamFactory::FileStreamFactory(Env::Configuration* pConfig)
        : m_FileStreamPool("IO/FileStream", sizeof(FileStream), 64 * 1024)
    {
        const IO::FixedPath currentDirectory = GetCurrentDirectory();
        m_ParentDirectory = pConfig->GetString("AssetDirectory", currentDirectory);
    }


    Result<Rc<IStream>, ResultCode> FileStreamFactory::OpenFileStream(StringSlice filename, OpenMode openMode)
    {
        const Rc fileStream = Rc<FileStream>::New(&m_FileStreamPool);
        const FixedPath fullPath = m_ParentDirectory / filename;
        const ResultCode result = fileStream->Open(fullPath, openMode);
        if (result != ResultCode::Success)
            return Err(result);

        return static_pointer_cast<IStream>(fileStream);
    }


    bool FileStreamFactory::FileExists(StringSlice filename)
    {
        return Platform::FileExists(filename);
    }


    FileAttributeFlags FileStreamFactory::GetFileAttributeFlags(StringSlice filename)
    {
        return Platform::GetFileAttributeFlags(filename);
    }
} // namespace FE::IO
