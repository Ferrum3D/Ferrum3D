#pragma once
#include <HAL/Common/BaseTypes.h>
#include <HAL/ImageFormat.h>

namespace FE::Graphics::HAL
{
    enum class ImageBindFlags
    {
        None = 0,

        ShaderRead = 1 << 0,
        UnorderedAccess = 1 << 1,

        Color = 1 << 2,
        Depth = 1 << 3,
        Stencil = 1 << 4,

        TransferRead = 1 << 5,
        TransferWrite = 1 << 6
    };

    FE_ENUM_OPERATORS(ImageBindFlags);

    enum class ImageAspect : uint8_t
    {
        Color,
        Depth,
        Stencil
    };

    enum class ImageAspectFlags : uint8_t
    {
        None,
        Color = 1 << static_cast<uint32_t>(ImageAspect::Color),
        Depth = 1 << static_cast<uint32_t>(ImageAspect::Depth),
        Stencil = 1 << static_cast<uint32_t>(ImageAspect::Stencil),
        DepthStencil = Depth | Stencil,
        All = Depth | Stencil | Color
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
} // namespace FE::Graphics::HAL
