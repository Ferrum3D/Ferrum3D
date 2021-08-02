#include <FeCore/Assets/AssetManager.h>
#include <FeCore/Console/FeLog.h>
#include <fstream>

namespace FE
{
    AssetManager::AssetManager() {}

    RawAsset AssetManager::LoadRawAsset(const std::string& fileName, size_t offset, size_t size)
    {
        return RawAsset(0, std::shared_ptr<void>(nullptr));
    }

    RawAsset AssetManager::LoadRawAsset(const std::string& fileName)
    {
        auto iter = m_CachedAssets.find(fileName);

        RawAssetWeak weakAsset;
        if (iter != m_CachedAssets.end())
        {
            weakAsset = iter->second;
        }

        auto strongRef = weakAsset.Ptr.lock();
        if (strongRef == nullptr)
        {
            auto asset               = LoadRawAssetImpl(fileName);
            m_CachedAssets[fileName] = RawAssetWeak(asset.GetSize(), asset.m_Ptr);
            return asset;
        }

        return RawAsset(weakAsset.Size, std::move(strongRef));
    }

    RawAsset AssetManager::LoadRawAssetImpl(const std::string& fileName)
    {
        std::ifstream file(fileName, std::ios::ate | std::ios::binary);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        char* array = new char[size + 1];
        array[size] = 0;
        file.read(array, size);
        FE_ASSERT_MSG(!file.fail(), "Failed to read file \"{}\"", fileName);
        return RawAsset(size, std::shared_ptr<void>(array));
    }
} // namespace FE
