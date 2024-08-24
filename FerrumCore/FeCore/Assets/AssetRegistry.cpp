#include <FeCore/Assets/AssetRegistry.h>
#include <FeCore/IO/FileHandle.h>

namespace FE::Assets
{
    void AssetRegistry::LoadAssetsFromFile(StringSlice fileName)
    {
        auto parentDir = IO::Directory::GetCurrentDirectory() / IO::Directory::GetParent(fileName);
        auto content = IO::File::ReadAllText(fileName);
        for (auto line : content.SplitLines())
        {
            const festd::vector split = line.Split();
            const StringSlice id = split[0];
            const StringSlice type = split[1];
            const StringSlice file = split[2];
            AddAsset(AssetID(id), AssetType(type), parentDir / file);
        }

        FE_LOG_MESSAGE("Successfully loaded Assets from {}", fileName);
    }

    void AssetRegistry::AddAsset(const AssetID& assetID, const AssetType& assetType, StringSlice fileName)
    {
        m_AssetFiles[assetID] = fileName;
        m_AssetTypes[assetID] = assetType;
    }

    void AssetRegistry::RemoveAsset(const AssetID& assetID)
    {
        m_AssetFiles.erase(assetID);
        m_AssetTypes.erase(assetID);
    }

    StringSlice AssetRegistry::GetAssetFilePath(const AssetID& assetID)
    {
        return m_AssetFiles[assetID];
    }

    bool AssetRegistry::HasAsset(const AssetID& assetID)
    {
        return m_AssetFiles.find(assetID) != m_AssetFiles.end();
    }

    AssetType AssetRegistry::GetAssetType(const AssetID& assetID)
    {
        return m_AssetTypes[assetID];
    }
} // namespace FE::Assets
