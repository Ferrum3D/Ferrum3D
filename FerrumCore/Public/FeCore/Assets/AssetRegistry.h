#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/Strings/String.h>
#include <FeCore/Containers/HashTables.h>

namespace FE::Assets
{
    //! \brief Development mode only asset registry.
    //!
    //! This class holds {AssetID : FileName} pairs to work with assets in development mode.
    //! It can be loaded from game assets directory or filled manually. The \ref IAssetProvider
    //! will use file names returned by this class to give \ref IAssetLoader streams to load
    //! assets from.
    class AssetRegistry : public Memory::RefCountedObjectBase
    {
        festd::unordered_dense_map<AssetID, String> m_AssetFiles;
        festd::unordered_dense_map<AssetID, AssetType> m_AssetTypes;

    public:
        FE_RTTI_Class(AssetRegistry, "35D62039-49D0-48AE-9AD7-ADA816B945F5");

        ~AssetRegistry() override = default;

        //! \brief Load Assets from a `FerrumAssetIndex` file.
        //!
        //! \param [in] fileName - The file to load assets from.
        void LoadAssetsFromFile(StringSlice fileName);

        //! \brief Add asset to registry.
        //!
        //! \param [in] assetID - ID of asset to add.
        //! \param [in] assetType - Type of asset to add.
        //! \param [in] fileName - File name of asset to add.
        void AddAsset(const AssetID& assetID, const AssetType& assetType, StringSlice fileName);

        //! \brief Remove asset from registry.
        //!
        //! \param [in] assetID - ID of asset to remove.
        void RemoveAsset(const AssetID& assetID);

        //! \brief Check if asset is registered.
        //!
        //! \param [in] assetID - ID of asset to check.
        //!
        //! \return True if asset is registered.
        bool HasAsset(const AssetID& assetID);

        //! \brief Get file path of asset.
        //!
        //! \param [in] assetID - ID of asset to get file path for.
        //!
        //! \return File path of asset.
        StringSlice GetAssetFilePath(const AssetID& assetID);

        //! \brief Get type of asset.
        //!
        //! \param [in] assetID - ID of asset to get type for.
        //!
        //! \return Type of asset.
        AssetType GetAssetType(const AssetID& assetID);
    };
} // namespace FE::Assets
