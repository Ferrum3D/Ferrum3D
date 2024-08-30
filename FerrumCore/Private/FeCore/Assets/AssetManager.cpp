#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Assets/IAssetLoader.h>

namespace FE::Assets
{
    AssetManager::AssetManager(IO::IStreamFactory* streamFactory)
        : m_StreamFactory(streamFactory)
    {
    }


    void AssetManager::RegisterAssetLoader(IAssetLoader* loader)
    {
        std::lock_guard lk{ m_Lock };
        const Env::Name assetType = loader->GetSpec().AssetTypeName;
        m_LoadersByType[assetType] = std::move(loader);
    }


    AssetStorage* AssetManager::LoadAsset(Env::Name assetName, Env::Name assetType, AssetLoadingFlags loadingFlags)
    {
        std::lock_guard lk{ m_Lock };
        IAssetLoader* loader = m_LoadersByType[assetType].Get();
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
        std::lock_guard lk{ m_Lock };
        m_LoadersByType.erase(assetType);
    }


    IAssetLoader* AssetManager::GetAssetLoader(Env::Name assetType)
    {
        std::lock_guard lk{ m_Lock };
        return m_LoadersByType[assetType].Get();
    }
} // namespace FE::Assets
