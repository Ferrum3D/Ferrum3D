#pragma once
#include <FeCore/Utils/UUID.h>

namespace FE::Assets
{
    //! \brief Alias for UUID used in asset system to identify asset types.
    //!
    //! Asset type is used to uniquely identify type of asset, e.g. texture asset, shader asset etc.
    //! Different asset loaders work with different asset types, load and initialize them in specific ways.
    using AssetType = UUID;

    //! \brief Alias for UUID used in asset system to identify assets.
    //!
    //! Asset ID is used to uniquely identify assets. Every shader, texture, 3D-model etc.
    //! in the application has its own ID.
    using AssetID = UUID;
} // namespace FE::Assets
