#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/IO/IStream.h>

namespace FE::Assets
{
    //! @brief Asset loader specification.
    //!
    //! This struct provides information about asset type and supported extensions for an asset loader.
    //! `m_fileExtension` is a file extension of ready and preprocessed asset that can be used in a production build.
    //! `m_sourceExtensions` is an optional list of extensions that this loader can load in development builds.
    //!
    //! For instance, image asset's `m_fileExtension` would be "dds" and `m_sourceExtensions` would contain
    //! strings like "png" or "jpg".
    struct AssetLoaderSpec final
    {
        Env::Name m_assetTypeName;
        StringSlice m_fileExtension;
        festd::span<const StringSlice> m_sourceExtensions;
    };


    //! @brief Asset loader interface.
    //!
    //! Asset loaders are responsible for loading assets from streams.
    struct IAssetLoader : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(IAssetLoader, "D0DE4F16-0C3C-44E9-9215-CBC6FC98EB22");

        ~IAssetLoader() override = default;

        //! @brief Get AssetLoaderSpec.
        [[nodiscard]] virtual const AssetLoaderSpec& GetSpec() const = 0;

        //! @brief Create storage for asset.
        //!
        //! The storage will have one strong reference. Remove the reference when you are done with it.
        //! Asset<T>::Load() will take ownership of the storage and will remove this reference automatically.
        virtual void CreateStorage(AssetStorage** ppStorage) = 0;

        virtual void LoadAsset(AssetStorage* storage, Env::Name assetName) = 0;
    };
} // namespace FE::Assets
