#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/Strings/String.h>
#include <unordered_map>

namespace FE::Assets
{
    //! \brief Development mode only asset registry.
    //!
    //! This class holds {AssetID : FileName} pairs to work with assets in development mode.
    //! It can be loaded from game assets directory or filled manually. The \ref IAssetProvider
    //! will use file names returned by this class to give \ref IAssetLoader streams to load
    //! assets from.
    class AssetRegistry : public Object<IObject>
    {
        UnorderedMap<AssetID, String> m_AssetFiles;
        UnorderedMap<AssetID, AssetType> m_AssetTypes;

    public:
        FE_CLASS_RTTI(AssetRegistry, "35D62039-49D0-48AE-9AD7-ADA816B945F5");

        void AddAsset(const AssetID& assetID, const AssetType& assetType, StringSlice fileName);
        void RemoveAsset(const AssetID& assetID);

        bool HasAsset(const AssetID& assetID);
        StringSlice GetAssetFilePath(const AssetID& assetID);
        AssetType GetAssetType(const AssetID& assetID);
    };
}
