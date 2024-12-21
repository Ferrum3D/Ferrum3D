#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/IAssetLoader.h>

namespace FE::Assets
{
    AssetManager::AssetManager(IO::IStreamFactory* streamFactory)
        : m_streamFactory(streamFactory)
    {
    }


    void AssetManager::RegisterAssetLoader(IAssetLoader* loader)
    {
        std::lock_guard lk{ m_lock };
        const Env::Name assetType = loader->GetSpec().m_assetTypeName;
        m_loadersByType[assetType] = loader;
    }


    AssetStorage* AssetManager::LoadAsset(Env::Name assetName, Env::Name assetType, AssetLoadingFlags loadingFlags)
    {
        std::lock_guard lk{ m_lock };
        IAssetLoader* loader = m_loadersByType[assetType].Get();
        AssetStorage* storage = nullptr;
        loader->CreateStorage(&storage);
        FE_Assert(storage);
        loader->LoadAsset(storage, assetName);

        if ((loadingFlags & AssetLoadingFlags::kSynchronous) == AssetLoadingFlags::kSynchronous)
        {
            while (!storage->IsCompletelyLoaded())
                _mm_pause();
        }

        return storage;
    }


    void AssetManager::UnregisterAssetLoader(Env::Name assetType)
    {
        std::lock_guard lk{ m_lock };
        m_loadersByType.erase(assetType);
    }


    IAssetLoader* AssetManager::GetAssetLoader(Env::Name assetType)
    {
        std::lock_guard lk{ m_lock };
        return m_loadersByType[assetType].Get();
    }
} // namespace FE::Assets
