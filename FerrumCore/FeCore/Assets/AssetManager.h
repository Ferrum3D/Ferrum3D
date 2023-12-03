#pragma once
#include <FeCore/Assets/IAssetManager.h>
#include <FeCore/Modules/ServiceLocator.h>

namespace FE::Assets
{
    class AssetManager : public ServiceLocatorImplBase<IAssetManager>
    {
        UnorderedMap<AssetType, Rc<IAssetLoader>> m_Loaders;
        Rc<IAssetProvider> m_Provider;

    public:
        FE_CLASS_RTTI(AssetManager, "753ACEEB-B5B8-4700-AE14-802DAFCA123C");

        ~AssetManager() override = default;

        void RegisterAssetLoader(Rc<IAssetLoader> loader) override;
        void RemoveAssetLoader(const AssetType& assetType) override;
        IAssetLoader* GetAssetLoader(const AssetType& assetType) override;
        void AttachAssetProvider(Rc<IAssetProvider> provider) override;
        void DetachAssetProvider() override;
        AssetStorage* LoadAsset(const AssetID& assetID) override;
    };
} // namespace FE::Assets
