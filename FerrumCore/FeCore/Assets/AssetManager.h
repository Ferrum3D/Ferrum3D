#pragma once
#include <FeCore/Assets/IAssetManager.h>
#include <FeCore/Modules/Singleton.h>

namespace FE::Assets
{
    class AssetManager : public SingletonImplBase<IAssetManager>
    {
        UnorderedMap<AssetType, Shared<IAssetLoader>> m_Loaders;
        Shared<IAssetProvider> m_Provider;

    public:
        FE_CLASS_RTTI(AssetManager, "753ACEEB-B5B8-4700-AE14-802DAFCA123C");

        ~AssetManager() override = default;

        void RegisterAssetLoader(Shared<IAssetLoader> loader) override;
        void RemoveAssetLoader(AssetType assetType) override;
        void AttachAssetProvider(Shared<IAssetProvider> provider) override;
        void DetachAssetProvider() override;
        AssetStorage* LoadAsset(const AssetID& assetID) override;
    };
} // namespace FE::Assets
