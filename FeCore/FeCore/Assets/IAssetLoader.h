#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/IO/IStream.h>

namespace FE::Assets
{
    class IAssetLoader : public IObject
    {
    public:
        FE_CLASS_RTTI(IAssetLoader, "D0DE4F16-0C3C-44E9-9215-CBC6FC98EB22");

        ~IAssetLoader() override = default;

        [[nodiscard]] virtual AssetType GetAssetType() const                    = 0;
        [[nodiscard]] virtual AssetStorage* CreateStorage()                     = 0;
        virtual void LoadAsset(AssetStorage* storage, IO::IStream* assetStream) = 0;
    };
} // namespace FE::Assets
