#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/IO/IStream.h>

namespace FE::Assets
{
    class IAssetProvider : public IObject
    {
    public:
        FE_CLASS_RTTI(IAssetProvider, "69148A5C-20A3-4255-868D-97DEE5319E84");

        ~IAssetProvider() override = default;

        virtual Shared<IO::IStream> CreateAssetLoadingStream(const AssetID& assetID) = 0;
        virtual AssetType GetAssetType(const AssetID& assetID)                       = 0;
    };
} // namespace FE::Assets
