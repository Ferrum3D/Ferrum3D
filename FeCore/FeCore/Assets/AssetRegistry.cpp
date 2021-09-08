#include <FeCore/Assets/AssetRegistry.h>

namespace FE::Assets
{
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
}
