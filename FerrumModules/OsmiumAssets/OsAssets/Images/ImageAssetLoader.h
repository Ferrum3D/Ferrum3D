#pragma once
#include <FeCore/Assets/IAssetLoader.h>

namespace FE::Osmium
{
    class ImageAssetLoader : public Assets::IAssetLoader
    {
    public:
        FE_RTTI_Class(ImageAssetLoader, "20BA2066-1FC7-46E4-9708-B45CE3EE177C");

        ~ImageAssetLoader() override = default;

        static Assets::AssetType AssetType;

        [[nodiscard]] Assets::AssetType GetAssetType() const override;
        [[nodiscard]] Assets::AssetStorage* CreateStorage() override;
        void LoadAsset(Assets::AssetStorage* storage, IO::IStream* assetStream) override;
        void LoadRawAsset(const eastl::vector<Assets::AssetMetadataField>& metadata, Assets::AssetStorage* storage,
                          IO::IStream* assetStream) override;
        void SaveAsset(Assets::AssetStorage* storage, IO::IStream* assetStream) override;
        eastl::vector<Assets::AssetMetadataField> GetAssetMetadataFields() override;
    };
} // namespace FE::Osmium
