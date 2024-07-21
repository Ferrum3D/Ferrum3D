#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/IO/IStream.h>

namespace FE::Assets
{
    //! \brief Asset provider interface.
    //!
    //! Asset provider is responsible for creating asset loading streams for given asset ID.
    //! It is used by IAssetManager.
    class IAssetProvider : public Memory::RefCountedObjectBase
    {
    public:
        FE_CLASS_RTTI(IAssetProvider, "69148A5C-20A3-4255-868D-97DEE5319E84");

        ~IAssetProvider() override = default;

        //! \brief Create asset loading stream for given asset ID.
        virtual Rc<IO::IStream> CreateAssetLoadingStream(const AssetID& assetID) = 0;

        //! \brief Returns asset type of given asset ID.
        //!
        //! \param [in] assetID - ID of asset.
        //!
        //! \return Asset type of given asset ID.
        virtual AssetType GetAssetType(const AssetID& assetID) = 0;
    };
} // namespace FE::Assets
