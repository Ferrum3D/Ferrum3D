#pragma once
#include <FeCore/Assets/IAssetLoader.h>

namespace FE::Osmium
{
    class ShaderAssetLoader : public Assets::IAssetLoader
    {
    public:
        FE_CLASS_RTTI(ShaderAssetLoader, "004C59CC-768C-418F-99E1-D95D73F52444");

        ~ShaderAssetLoader() override = default;

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
