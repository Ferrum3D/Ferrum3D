#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/IAssetLoader.h>
#include <FeCore/Assets/IAssetProvider.h>

namespace FE::Assets
{
    void AssetManager::RegisterAssetLoader(Shared<IAssetLoader> loader)
    {
        m_Loaders[loader->GetAssetType()] = std::move(loader);
    }

    void AssetManager::AttachAssetProvider(Shared<IAssetProvider> provider)
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
        auto loader   = m_Loaders[type];
        auto* storage = loader->CreateStorage();
        loader->LoadAsset(storage, stream.GetRaw());
        return storage;
    }
} // namespace FE::Assets
