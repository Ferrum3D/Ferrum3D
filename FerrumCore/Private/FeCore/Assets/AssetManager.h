#pragma once
#include <FeCore/Assets/IAssetManager.h>
#include <FeCore/Containers/HashTables.h>
#include <FeCore/IO/StreamFactory.h>
#include <FeCore/Modules/ServiceLocator.h>

namespace FE::Assets
{
    class AssetManager final : public IAssetManager
    {
        IO::IStreamFactory* m_StreamFactory;
        SpinLock m_Lock;
        festd::unordered_dense_map<Env::Name, Rc<IAssetLoader>> m_LoadersByType;

    public:
        FE_RTTI_Class(AssetManager, "753ACEEB-B5B8-4700-AE14-802DAFCA123C");

        AssetManager(IO::IStreamFactory* streamFactory);
        ~AssetManager() override = default;

        void RegisterAssetLoader(IAssetLoader* loader) override;
        void UnregisterAssetLoader(Env::Name assetType) override;
        IAssetLoader* GetAssetLoader(Env::Name assetType) override;
        AssetStorage* LoadAsset(Env::Name assetName, Env::Name assetType, AssetLoadingFlags loadingFlags) override;
    };
} // namespace FE::Assets
