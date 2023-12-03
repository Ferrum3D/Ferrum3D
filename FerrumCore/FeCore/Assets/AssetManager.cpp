#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/Assets/IAssetProvider.h>

namespace FE::Assets
{
    void AssetManager::RegisterAssetLoader(Rc<IAssetLoader> loader)
    {
        m_Loaders[loader->GetAssetType()] = std::move(loader);
    }

    void AssetManager::AttachAssetProvider(Rc<IAssetProvider> provider)
    {
        m_Provider = std::move(provider);
    }

    void AssetManager::DetachAssetProvider()
    {
        m_Provider.Reset();
    }

    AssetStorage* AssetManager::LoadAsset(const AssetID& assetID)
    {
        auto type     = m_Provider->GetAssetType(assetID);
        auto stream   = m_Provider->CreateAssetLoadingStream(assetID);
        auto loader   = m_Loaders[type].Get();
        auto* storage = loader->CreateStorage();
        storage->AddStrongRef();
        loader->LoadAsset(storage, stream.Get());
        return storage;
    }

    void AssetManager::RemoveAssetLoader(const AssetType& assetType)
    {
        m_Loaders.erase(assetType);
    }

    IAssetLoader* AssetManager::GetAssetLoader(const AssetType& assetType)
    {
        return m_Loaders[assetType].Get();
    }
} // namespace FE::Assets
