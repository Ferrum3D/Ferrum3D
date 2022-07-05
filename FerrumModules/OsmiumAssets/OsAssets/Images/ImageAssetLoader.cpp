#include <FeCore/Console/FeLog.h>
#include <FeCore/Containers/List.h>
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
        auto* p = GlobalAllocator<HeapAllocator>::Get().Allocate(sizeof(ImageAssetStorage), MaximumAlignment, FE_SRCPOS());
        return new (p) ImageAssetStorage(this);
    }

    void ImageAssetLoader::LoadAsset(Assets::AssetStorage* storage, IO::IStream* assetStream)
    {
        auto* imageStorage = static_cast<ImageAssetStorage*>(storage);
        auto length        = assetStream->Length();
        List<UInt8> buffer(length, 0);
        assetStream->ReadToBuffer(buffer.Data(), length);

        auto result = Internal::LoadImageFromMemory(buffer.Data(), length, imageStorage->m_Width, imageStorage->m_Height);
        if (result.IsOk())
        {
            imageStorage->m_Data = result.Unwrap();
            return;
        }

        FE_UNREACHABLE("{}", result.GetError());
    }
} // namespace FE::Osmium
