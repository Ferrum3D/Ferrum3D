#include <Core/IO/AssetFile.h>
#include <Core/IO/FileStream.h>
#include <Core/Serialization/Document.h>

namespace FE::IO
{
    bool LoadAssetDefinition(const Path& path, AssetFile& result)
    {
        auto file = FileStream::Open(path, OpenMode::kReadOnly);
        if (!file)
            return false;

        AssetFile candidate;
        if (Serialization::LoadDocument(**file, candidate) != Serialization::ResultCode::kSuccess)
            return false;

        result = std::move(candidate);
        return true;
    }


    bool SaveAssetDefinition(const Path& path, const AssetFile& value)
    {
        if (Directory::Create(PathView(path).parent_directory()) != ResultCode::kSuccess)
            return false;

        auto file = FileStream::Open(path, OpenMode::kCreate);
        if (!file)
            return false;

        Serialization::JsonFormat format;
        Serialization::SerializationContext context(file->Get(), format);
        return context.Store(value) == Serialization::ResultCode::kSuccess;
    }
} // namespace FE::IO
