#include <Core/IO/FileStream.h>
#include <Core/IO/Platform/PlatformFile.h>
#include <Core/IO/StreamFactory.h>

namespace FE::IO
{
    FileStreamFactory::FileStreamFactory(Env::Configuration* config)
    {
        const Path currentDirectory = Directory::GetCurrentDirectory();
        m_parentDirectory = config->GetString("AssetDirectory", currentDirectory);
        Directory::SetCurrentDirectory(m_parentDirectory);
    }


    festd::expected<Rc<IStream>, ResultCode> FileStreamFactory::OpenFileStream(const festd::string_view filename,
                                                                               const OpenMode openMode)
    {
        FE_PROFILER_ZONE();

        const Rc fileStream = Memory::DefaultNew<FileStream>();
        const Path fullPath = m_parentDirectory / filename;
        const ResultCode result = fileStream->Open(fullPath, openMode);
        if (result != ResultCode::kSuccess)
            return festd::unexpected(result);

        return static_pointer_cast<IStream>(fileStream);
    }


    festd::expected<Rc<IStream>, ResultCode> FileStreamFactory::OpenUnbufferedFileStream(const festd::string_view filename,
                                                                                         const OpenMode openMode)
    {
        FE_PROFILER_ZONE();

        const Rc fileStream = Memory::DefaultNew<FileStream>();
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
