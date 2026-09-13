#include <AssetBuilder/AssetFile.h>

#include <Core/IO/FileStream.h>
#include <Core/Serialization/JsonSerialization.h>

namespace FE::AssetBuilder
{
    bool LoadAssetFile(const IO::Path& path, AssetFile& result)
    {
        auto fileResult = IO::FileStream::Open(path, IO::OpenMode::kReadOnly);
        if (!fileResult)
            return false;

        Serialization::JsonFormat format;
        Serialization::DeserializationContext context(fileResult->Get(), format);
        return context.Load(result) == Serialization::ResultCode::kSuccess;
    }


    bool SaveAssetFile(const IO::Path& path, const AssetFile& assetFile)
    {
        const IO::ResultCode directoryResult = IO::Directory::Create(IO::PathView(path).parent_directory());
        if (directoryResult != IO::ResultCode::kSuccess)
            return false;

        auto fileResult = IO::FileStream::Open(path, IO::OpenMode::kCreate);
        if (!fileResult)
            return false;

        Serialization::JsonFormat format;
        Serialization::SerializationContext context(fileResult->Get(), format);
        return context.Store(assetFile) == Serialization::ResultCode::kSuccess;
    }
} // namespace FE::AssetBuilder
