#pragma once
#include <FeCore/IO/IStreamFactory.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Modules/Configuration.h>

namespace FE::IO
{
    struct FileStreamFactory final : public IStreamFactory
    {
        FE_RTTI_Class(FileStreamFactory, "3F973B26-1330-404A-BF05-CE0B63306871");

        explicit FileStreamFactory(Env::Configuration* pConfig);

        festd::expected<Rc<IStream>, ResultCode> OpenFileStream(festd::string_view filename, OpenMode openMode) override;
        festd::expected<Rc<IStream>, ResultCode> OpenUnbufferedFileStream(festd::string_view filename,
                                                                          OpenMode openMode) override;
        bool FileExists(festd::string_view filename) override;
        FileAttributeFlags GetFileAttributeFlags(festd::string_view filename) override;

    private:
        Path m_parentDirectory;
        Memory::LockedMemoryResource<Memory::PoolAllocator, Threading::SpinLock> m_fileStreamPool;
    };
} // namespace FE::IO
