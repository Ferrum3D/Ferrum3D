#pragma once
#include <FeCore/Assets/AssetCommon.h>
#include <FeCore/Memory/Memory.h>

namespace FE::Assets
{
    class IAssetProvider;
    class IAssetLoader;
    class AssetStorage;

    //! \brief Asset manager interface.
    //!
    //! This interface is used to load assets. It uses the attached providers and loaders to get data streams
    //! and initialize asset storages.
    class IAssetManager : public IObject
    {
    public:
        FE_CLASS_RTTI(IAssetManager, "7FC7D500-5CE4-4BF7-8030-1A5A4A16832C");

        ~IAssetManager() override = default;

        //! \brief Register an instance of IAssetLoader.
        //!
        //! \param [in] loader - The loader to register.
        virtual void RegisterAssetLoader(Rc<IAssetLoader> loader) = 0;

        //! \brief Remove an instance of IAssetLoader.
        //!
        //! This is useful to clean up the AssetManager after a module that attached its own asset loaders.
        //!
        //! \param [in] assetType - The type of the asset associated with the loader to remove.
        virtual void RemoveAssetLoader(const AssetType& assetType) = 0;

        //! \brief Get registered loader for a specified asset type.
        //!
        //! \param [in] assetType - The type of the asset associated with the loader to return.
        //!
        //! \return The asset loader that loads assets with specified type.
        virtual IAssetLoader* GetAssetLoader(const AssetType& assetType) = 0;

        //! \brief Register an instance of IAssetProvider.
        //!
        //! \param[in] provider - The provider to register.
        virtual void AttachAssetProvider(Rc<IAssetProvider> provider) = 0;

        //! \brief Detach the attached instance of IAssetProvider.
        virtual void DetachAssetProvider() = 0;

        //! \brief Load asset synchronously on this thread.
        //!
        //! \param [in] assetID - ID of asset to load.
        //!
        //! \return Allocated and fully loaded AssetStorage.
        virtual AssetStorage* LoadAsset(const AssetID& assetID) = 0;
    };
} // namespace FE::Assets
