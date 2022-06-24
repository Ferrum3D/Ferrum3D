#pragma once
#include <FeCore/Assets/AssetRegistry.h>
#include <FeCore/Assets/IAssetProvider.h>

namespace FE::Assets
{
    //! \brief Asset provider that loads assets directly from disk.
    //!
    //! This class is used to load assets directly from disk. It can only be used in development mode.
    class AssetProviderDev : public Object<IAssetProvider>
    {
        Shared<AssetRegistry> m_Registry;

    public:
        FE_CLASS_RTTI(AssetProviderDev, "B9E55773-ED2D-4103-A4C7-F2F4BE045A90");

        ~AssetProviderDev() override = default;

        //! \brief Attach registry to this provider.
        //!
        //! \param [in] registry - Registry to attach.
        //!
        //! \see AssetRegistry
        void AttachRegistry(const Shared<AssetRegistry>& registry);
        
        //! \brief Detach registry from this provider.
        //!
        //! \see AssetRegistry
        void DetachRegistry();

        Shared<IO::IStream> CreateAssetLoadingStream(const AssetID& assetID) override;
        AssetType GetAssetType(const AssetID& assetID) override;
    };
} // namespace FE::Assets
