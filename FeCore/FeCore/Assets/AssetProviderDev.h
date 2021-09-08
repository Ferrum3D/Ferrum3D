#pragma once
#include <FeCore/Assets/AssetRegistry.h>
#include <FeCore/Assets/IAssetProvider.h>

namespace FE::Assets
{
    class AssetProviderDev : public Object<IAssetProvider>
    {
        Shared<AssetRegistry> m_Registry;

    public:
        FE_CLASS_RTTI(AssetProviderDev, "B9E55773-ED2D-4103-A4C7-F2F4BE045A90");

        ~AssetProviderDev() override = default;

        void AttachRegistry(const Shared<AssetRegistry>& registry);
        void DetachRegistry();

        Shared<IO::IStream> CreateAssetLoadingStream(const AssetID& assetID) override;
        AssetType GetAssetType(const AssetID& assetID) override;
    };
} // namespace FE::Assets
