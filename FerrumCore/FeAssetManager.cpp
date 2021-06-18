#include <fstream>
#include "FeAssetManager.h"
#include "FeLog.h"

namespace Ferrum
{
    FeAssetManager::FeAssetManager() {

    }

    FeRawAsset FeAssetManager::LoadRawAsset(const std::string& fileName, size_t offset, size_t size) {
        return FeRawAsset(0, std::shared_ptr<void>(nullptr));
    }

    FeRawAsset FeAssetManager::LoadRawAsset(const std::string& fileName) {
        auto iter = m_CachedAssets.find(fileName);

        FeRawAssetWeak weakAsset;
        if (iter != m_CachedAssets.end()) {
            weakAsset = iter->second;
        }

        auto strongRef = weakAsset.Ptr.lock();
        if (strongRef == nullptr) {
            auto asset = LoadRawAssetImpl(fileName);
            m_CachedAssets[fileName] = FeRawAssetWeak(asset.GetSize(), asset.m_Ptr);
            return asset;
        }

        return FeRawAsset(weakAsset.Size, std::move(strongRef));
    }

    FeRawAsset FeAssetManager::LoadRawAssetImpl(const std::string& fileName) {
#ifdef FE_DEBUG
        std::ifstream file(fileName, std::ios::ate | std::ios::binary);
        auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        auto buffer = std::shared_ptr<void>(new char[size]);
        file.read((char*)buffer.get(), size);
        FE_ASSERT_MSG(!file.fail(), "Failed to read file \"{}\"", fileName);
        return FeRawAsset(size, std::move(buffer));
#else
#   error Release-mode asset manager isn't implemented yet
#endif
    }
}
