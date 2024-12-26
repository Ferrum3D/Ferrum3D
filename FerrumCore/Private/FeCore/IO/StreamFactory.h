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

        festd::expected<Rc<IStream>, ResultCode> OpenFileStream(StringSlice filename, OpenMode openMode) override;
        bool FileExists(StringSlice filename) override;
        FileAttributeFlags GetFileAttributeFlags(StringSlice filename) override;

    private:
        FixedPath m_parentDirectory;
        Memory::LockedMemoryResource<Memory::PoolAllocator, Threading::SpinLock> m_fileStreamPool;
    };
} // namespace FE::IO
