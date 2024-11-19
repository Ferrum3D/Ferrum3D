#pragma once
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/IO/IStreamFactory.h>

namespace FE::Graphics
{
    struct MeshAssetLoader : public Assets::IAssetLoader
    {
        FE_RTTI_Class(MeshAssetLoader, "7A4E500E-9AEC-4B3A-A6F0-16B47299ECA2");

        MeshAssetLoader(IO::IStreamFactory* streamFactory);
        ~MeshAssetLoader() override = default;

        const Assets::AssetLoaderSpec& GetSpec() const override;

        void CreateStorage(Assets::AssetStorage** ppStorage) override;

        void LoadAsset(Assets::AssetStorage* storage, Env::Name assetName) override;

    private:
        Assets::AssetLoaderSpec m_spec;
        IO::IStreamFactory* m_streamFactory = nullptr;
    };
} // namespace FE::Graphics
