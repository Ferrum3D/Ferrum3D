#include <FeCore/Utils/BinarySerializer.h>
#include <OsAssets/Shaders/ShaderAssetLoader.h>
#include <OsAssets/Shaders/ShaderAssetStorage.h>

namespace FE::Osmium
{
    Assets::AssetType ShaderAssetLoader::AssetType = Assets::AssetType("31DB0E12-F802-4DA1-9C88-6D1A8BB391DA");

    Assets::AssetType ShaderAssetLoader::GetAssetType() const
    {
        return AssetType;
    }

    Assets::AssetStorage* ShaderAssetLoader::CreateStorage()
    {
        auto* p = GlobalAllocator<HeapAllocator>::Get().Allocate(sizeof(ShaderAssetStorage), MaximumAlignment, FE_SRCPOS());
        return new (p) ShaderAssetStorage(this);
    }

    void ShaderAssetLoader::SaveAsset(Assets::AssetStorage* storage, IO::IStream* assetStream)
    {
        auto* shaderStorage = static_cast<ShaderAssetStorage*>(storage);
        BinarySerializer serializer(assetStream);
        serializer.WriteString(shaderStorage->GetSourceCode());
    }

    void ShaderAssetLoader::LoadAsset(Assets::AssetStorage* storage, IO::IStream* assetStream)
    {
        auto* shaderStorage = static_cast<ShaderAssetStorage*>(storage);
        BinarySerializer serializer(assetStream);
        serializer.ReadString(shaderStorage->SourceCode);
    }

    void ShaderAssetLoader::LoadRawAsset(const List<Assets::AssetMetadataField>& /* metadata */, Assets::AssetStorage* storage,
                                         IO::IStream* assetStream)
    {
        auto* shaderStorage       = static_cast<ShaderAssetStorage*>(storage);
        shaderStorage->SourceCode = String(assetStream->Length(), '\0');
        assetStream->ReadToBuffer(shaderStorage->SourceCode.Data(), shaderStorage->SourceCode.Size());
    }

    List<Assets::AssetMetadataField> ShaderAssetLoader::GetAssetMetadataFields()
    {
        return {};
    }
} // namespace FE::Osmium
