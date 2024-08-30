#pragma once
#include <FeCore/IO/IStreamFactory.h>
#include <FeCore/Memory/PoolAllocator.h>
#include <FeCore/Modules/Configuration.h>

namespace FE::IO
{
    class FileStreamFactory : public IStreamFactory
    {
        FixedPath m_ParentDirectory;
        Memory::LockedMemoryResource<Memory::PoolAllocator, SpinLock> m_FileStreamPool;

    public:
        FE_RTTI_Class(FileStreamFactory, "3F973B26-1330-404A-BF05-CE0B63306871");

        FileStreamFactory(Env::Configuration* pConfig);

        Result<Rc<IStream>, ResultCode> OpenFileStream(StringSlice filename, OpenMode openMode) override;
        bool FileExists(StringSlice filename) override;
        FileAttributeFlags GetFileAttributeFlags(StringSlice filename) override;
    };
} // namespace FE::IO
