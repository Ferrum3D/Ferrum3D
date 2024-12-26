#pragma once
#include <FeCore/IO/IStream.h>

namespace FE::IO
{
    struct IStreamFactory : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(IStreamFactory, "4BFCAD35-3DA3-4115-B4AD-96086AD97A8C");

        virtual festd::expected<Rc<IStream>, ResultCode> OpenFileStream(StringSlice filename, OpenMode openMode) = 0;
        virtual bool FileExists(StringSlice filename) = 0;
        virtual FileAttributeFlags GetFileAttributeFlags(StringSlice filename) = 0;
    };
} // namespace FE::IO
