#pragma once
#include <Core/IO/IStreamFactory.h>
#include <Core/Memory/PoolAllocator.h>
#include <Core/Modules/Configuration.h>

namespace FE::IO
{
    struct FileStreamFactory final : public IStreamFactory
    {
        FE_RTTI("3F973B26-1330-404A-BF05-CE0B63306871");

        explicit FileStreamFactory(Env::Configuration* config);

        festd::expected<Rc<IStream>, ResultCode> OpenFileStream(festd::string_view filename, OpenMode openMode) override;
        festd::expected<Rc<IStream>, ResultCode> OpenUnbufferedFileStream(festd::string_view filename,
                                                                          OpenMode openMode) override;
        bool FileExists(festd::string_view filename) override;
        FileAttributeFlags GetFileAttributeFlags(festd::string_view filename) override;

    private:
        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        Path m_parentDirectory;
    };
} // namespace FE::IO
