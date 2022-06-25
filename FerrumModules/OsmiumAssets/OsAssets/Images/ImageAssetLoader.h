#pragma once
#include <FeCore/Assets/IAssetLoader.h>

namespace FE::Osmium
{
    class ImageAssetLoader : public Object<Assets::IAssetLoader>
    {
    public:
        FE_CLASS_RTTI(ImageAssetLoader, "20BA2066-1FC7-46E4-9708-B45CE3EE177C");

        ~ImageAssetLoader() override = default;

        static Assets::AssetType AssetType;

        [[nodiscard]] Assets::AssetType GetAssetType() const override;
        [[nodiscard]] Assets::AssetStorage* CreateStorage() override;
        void LoadAsset(Assets::AssetStorage* storage, IO::IStream* assetStream) override;
    };
} // namespace FE::Osmium
