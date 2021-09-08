#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Assets/AssetCommon.h>

namespace FE::Assets
{
    class IAssetProvider;
    class IAssetLoader;
    class AssetStorage;

    class IAssetManager : public IObject
    {
    public:
        FE_CLASS_RTTI(IAssetManager, "7FC7D500-5CE4-4BF7-8030-1A5A4A16832C");

        ~IAssetManager() override = default;

        virtual void RegisterAssetLoader(Shared<IAssetLoader> loader)     = 0;
        virtual void AttachAssetProvider(Shared<IAssetProvider> provider) = 0;
        virtual void DetachAssetProvider()                                = 0;

        //! \brief Load asset synchronously on this thread.
        //!
        //! \param [in] assetID - ID of asset to load.
        //!
        //! \return Allocated and fully loaded AssetStorage.
        virtual AssetStorage* LoadAsset(const AssetID& assetID) = 0;
    };
} // namespace FE::Assets
