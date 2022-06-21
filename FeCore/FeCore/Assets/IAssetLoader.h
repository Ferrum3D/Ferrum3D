#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/IO/IStream.h>

namespace FE::Assets
{
    //! \brief Asset loader interface.
    //!
    //! Asset loaders are responsible for loading assets from streams.
    class IAssetLoader : public IObject
    {
    public:
        FE_CLASS_RTTI(IAssetLoader, "D0DE4F16-0C3C-44E9-9215-CBC6FC98EB22");

        ~IAssetLoader() override = default;

        //! \brief Get type of asset that this loader can load.
        [[nodiscard]] virtual AssetType GetAssetType() const = 0;

        //! \brief Create storage for asset.
        //!
        //! The storage will have one strong reference. Remove the reference when you are done with it.
        //! Asset<T>::Load() will take ownership of the storage and will remove this reference automatically.
        [[nodiscard]] virtual AssetStorage* CreateStorage() = 0;

        //! \brief Load asset from stream.
        //!
        //! Load asset from stream and store it in storage synchronously on this thread.
        //!
        //! \param [in] storage - Storage to load asset into.
        //! \param [in] assetStream - Stream to load asset from.
        virtual void LoadAsset(AssetStorage* storage, IO::IStream* assetStream) = 0;
    };
} // namespace FE::Assets
