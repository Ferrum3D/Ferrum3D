#pragma once
#include <OsGPU/Common/BaseTypes.h>
#include <OsGPU/Image/ImageFormat.h>

namespace FE::Osmium
{
    enum class ImageBindFlags
    {
        None            = 0,
        ShaderRead      = 1 << 0,
        ShaderWrite     = 1 << 1,
        ShaderReadWrite = ShaderRead | ShaderWrite,

        RenderTarget = 1 << 2,
        Depth        = 1 << 3,
        Stencil      = 1 << 4,

        TransferRead  = 1 << 5,
        TransferWrite = 1 << 6,

        UnorderedAccess = 1 << 7
    };

    FE_ENUM_OPERATORS(ImageBindFlags);

    enum class ImageAspect
    {
        RenderTarget,
        Depth,
        Stencil
    };

    enum class ImageAspectFlags
    {
        None,
        RenderTarget = 1 << static_cast<UInt32>(ImageAspect::RenderTarget),
        Depth        = 1 << static_cast<UInt32>(ImageAspect::Depth),
        Stencil      = 1 << static_cast<UInt32>(ImageAspect::Stencil),
        DepthStencil = Depth | Stencil,
        All          = Depth | Stencil | RenderTarget
    };

    FE_ENUM_OPERATORS(ImageAspectFlags);

    enum class ImageDim
    {
        None,
        Image1D,
        Image2D,
        Image3D,
        ImageCubemap
    };
} // namespace FE::Osmium
