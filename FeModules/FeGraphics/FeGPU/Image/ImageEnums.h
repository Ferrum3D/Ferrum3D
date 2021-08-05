#pragma once
#include <FeGPU/Common/BaseTypes.h>
#include <FeGPU/Image/ImageFormat.h>

namespace FE::GPU
{
    // clang-format off
    FE_ENUM(ImageBindFlags)
    {
        None            = 0,
        ShaderRead      = 1 << 0,
        ShaderWrite     = 1 << 1,
        ShaderReadWrite = ShaderRead | ShaderWrite,

        Color           = 1 << 2,
        Depth           = 1 << 3,
        Stencil         = 1 << 4,

        TransferRead    = 1 << 5,
        TransferWrite   = 1 << 6
    };
    // clang-format on

    // clang-format off
    FE_ENUM(ImageAspect)
    {
        None,
        Color        = 1 << 0,
        Depth        = 1 << 1,
        Stencil      = 1 << 2,
        DepthStencil = Depth | Stencil,
        All          = Color | Depth | Stencil
    };
    // clang-format on

    enum class ImageDim
    {
        None,
        Image1D,
        Image2D,
        Image3D,
        ImageCubemap
    };
} // namespace FE::GPU
