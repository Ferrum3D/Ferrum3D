#pragma once
#include <FeCore/Assets/IAssetLoader.h>

namespace FE::Osmium
{
    class ImageAssetLoader : public Object<Assets::IAssetLoader>
    {
    public:
        ~ImageAssetLoader() override = default;

        static Assets::AssetType AssetType;

        [[nodiscard]] Assets::AssetType GetAssetType() const override;
        [[nodiscard]] Assets::AssetStorage* CreateStorage() override;
        void LoadAsset(Assets::AssetStorage* storage, IO::IStream* assetStream) override;
    };
} // namespace FE::Osmium
