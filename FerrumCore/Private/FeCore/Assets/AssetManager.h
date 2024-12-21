#pragma once
#include <FeCore/Assets/IAssetManager.h>
#include <festd/unordered_map.h>
#include <FeCore/IO/StreamFactory.h>
#include <FeCore/Modules/ServiceLocator.h>

namespace FE::Assets
{
    struct AssetManager final : public IAssetManager
    {
        FE_RTTI_Class(AssetManager, "753ACEEB-B5B8-4700-AE14-802DAFCA123C");

        AssetManager(IO::IStreamFactory* streamFactory);
        ~AssetManager() override = default;

        void RegisterAssetLoader(IAssetLoader* loader) override;
        void UnregisterAssetLoader(Env::Name assetType) override;
        IAssetLoader* GetAssetLoader(Env::Name assetType) override;
        AssetStorage* LoadAsset(Env::Name assetName, Env::Name assetType, AssetLoadingFlags loadingFlags) override;

    private:
        IO::IStreamFactory* m_streamFactory;
        Threading::SpinLock m_lock;
        festd::unordered_dense_map<Env::Name, Rc<IAssetLoader>> m_loadersByType;
    };
} // namespace FE::Assets
