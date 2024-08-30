#pragma once
#include <HAL/Common/BaseTypes.h>
#include <HAL/ImageFormat.h>

namespace FE::Graphics::HAL
{
    enum class ImageBindFlags
    {
        kNone = 0,

        kShaderRead = 1 << 0,
        kUnorderedAccess = 1 << 1,

        kColor = 1 << 2,
        kDepth = 1 << 3,
        kStencil = 1 << 4,

        kTransferRead = 1 << 5,
        kTransferWrite = 1 << 6,
    };

    FE_ENUM_OPERATORS(ImageBindFlags);


    enum class ImageDim
    {
        kImage1D,
        kImage2D,
        kImage3D,
        kImageCubemap,
    };
} // namespace FE::Graphics::HAL
