#pragma once
#include <FeCore/Assets/AssetCommon.h>
#include <FeCore/Memory/Memory.h>
#include <FeCore/Modules/Environment.h>
#include <FeCore/RTTI/RTTI.h>

namespace FE::Assets
{
    struct IAssetLoader;
    class AssetStorage;


    enum class AssetLoadingFlags
    {
        kNone = 0,
        kSynchronous = 1 << 0,
    };

    FE_ENUM_OPERATORS(AssetLoadingFlags);


    //! @brief Asset manager interface.
    //!
    //! This interface is used to load assets. It uses the attached providers and loaders to get data streams
    //! and initialize asset storages.
    struct IAssetManager : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(IAssetManager, "7FC7D500-5CE4-4BF7-8030-1A5A4A16832C");

        ~IAssetManager() override = default;

        //! @brief Register an instance of IAssetLoader.
        virtual void RegisterAssetLoader(IAssetLoader* loader) = 0;

        //! @brief Unregister an instance of IAssetLoader by its asset type.
        virtual void UnregisterAssetLoader(Env::Name assetType) = 0;

        //! @brief Get registered loader for a specified asset type.
        virtual IAssetLoader* GetAssetLoader(Env::Name assetType) = 0;

        virtual AssetStorage* LoadAsset(Env::Name assetName, Env::Name assetType, AssetLoadingFlags loadingFlags) = 0;
    };
} // namespace FE::Assets
