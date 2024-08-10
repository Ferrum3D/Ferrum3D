#include <FeCore/Console/FeLog.h>
#include <OsAssets/Images/ImageAssetLoader.h>
#include <OsAssets/Images/ImageAssetStorage.h>
#include <OsAssets/Images/ImageLoaderImpl.h>

namespace FE::Osmium
{
    Assets::AssetType ImageAssetLoader::AssetType = Assets::AssetType("B4211F00-8E8D-4F58-96B6-35EF4C3D66D0");

    Assets::AssetType ImageAssetLoader::GetAssetType() const
    {
        return AssetType;
    }

    Assets::AssetStorage* ImageAssetLoader::CreateStorage()
    {
        return Memory::DefaultNew<ImageAssetStorage>(this);
    }

    void ImageAssetLoader::SaveAsset(Assets::AssetStorage* storage, IO::IStream* assetStream)
    {
        auto* imageStorage = static_cast<ImageAssetStorage*>(storage);
        Internal::WriteImageToStream(imageStorage->Data(), imageStorage->Width(), imageStorage->Height(), assetStream);
    }

    void ImageAssetLoader::LoadAsset(Assets::AssetStorage* storage, IO::IStream* assetStream)
    {
        LoadRawAsset({}, storage, assetStream);
    }

    void ImageAssetLoader::LoadRawAsset(const eastl::vector<Assets::AssetMetadataField>& /* metadata */,
                                        Assets::AssetStorage* storage, IO::IStream* assetStream)
    {
        auto* imageStorage = static_cast<ImageAssetStorage*>(storage);
        const uint32_t length = static_cast<uint32_t>(assetStream->Length());
        eastl::vector<uint8_t> buffer(length, 0);
        assetStream->ReadToBuffer(buffer.data(), length);

        int32_t channels;
        auto result =
            Internal::LoadImageFromMemory(buffer.data(), length, imageStorage->m_Width, imageStorage->m_Height, channels);
        if (result.IsOk())
        {
            imageStorage->m_Data = result.Unwrap();
            return;
        }

        FE_UNREACHABLE("{}", result.UnwrapErr());
    }

    eastl::vector<Assets::AssetMetadataField> ImageAssetLoader::GetAssetMetadataFields()
    {
        return {};
    }
} // namespace FE::Osmium
