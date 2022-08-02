#pragma once
#include <FeCore/Assets/IAssetLoader.h>

namespace FE::Osmium
{
    class MeshAssetLoader : public Object<Assets::IAssetLoader>
    {
        List<Assets::AssetMetadataField> m_MetadataFields;

    public:
        FE_CLASS_RTTI(MeshAssetLoader, "7A4E500E-9AEC-4B3A-A6F0-16B47299ECA2");

        ~MeshAssetLoader() override = default;

        static Assets::AssetType AssetType;

        [[nodiscard]] Assets::AssetType GetAssetType() const override;
        [[nodiscard]] Assets::AssetStorage* CreateStorage() override;
        void LoadAsset(Assets::AssetStorage* storage, IO::IStream* assetStream) override;
        void LoadRawAsset(const List<Assets::AssetMetadataField>& metadata, Assets::AssetStorage* storage,
                          IO::IStream* assetStream) override;
        void SaveAsset(Assets::AssetStorage* storage, IO::IStream* assetStream) override;
        ArraySlice<Assets::AssetMetadataField> GetAssetMetadataFields() override;
    };
} // namespace FE::Osmium
