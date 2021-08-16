#include <FeGPU/Image/IImage.h>

namespace FE::GPU
{
    ImageDesc ImageDesc::Img1D(ImageBindFlags bindFlags, UInt32 width, Format format)
    {
        return ImageDesc::Img1DArray(bindFlags, width, 1, format);
    }

    ImageDesc ImageDesc::Img1DArray(ImageBindFlags bindFlags, UInt32 width, UInt16 arraySize, Format format)
    {
        ImageDesc desc{};
        desc.BindFlags   = bindFlags;
        desc.Dimension   = ImageDim::Image1D;
        desc.ImageFormat = format;
        desc.ImageSize   = { width, 1, 1 };
        desc.ArraySize   = arraySize;
        return desc;
    }

    ImageDesc ImageDesc::Img2D(ImageBindFlags bindFlags, UInt32 width, UInt32 height, Format format)
    {
        return ImageDesc::Img2DArray(bindFlags, width, height, 1, format);
    }

    ImageDesc ImageDesc::Img2DArray(ImageBindFlags bindFlags, UInt32 width, UInt32 height, UInt16 arraySize, Format format)
    {
        ImageDesc desc{};
        desc.BindFlags   = bindFlags;
        desc.Dimension   = ImageDim::Image2D;
        desc.ImageSize   = { width, height, 1 };
        desc.ArraySize   = arraySize;
        desc.ImageFormat = format;
        return desc;
    }

    ImageDesc ImageDesc::ImgCubemap(ImageBindFlags bindFlags, UInt32 width, Format format)
    {
        return ImageDesc::ImgCubemapArray(bindFlags, width, 1, format);
    }

    ImageDesc ImageDesc::ImgCubemapArray(ImageBindFlags bindFlags, UInt32 width, UInt16 arraySize, Format format)
    {
        ImageDesc desc;
        desc.BindFlags   = bindFlags;
        desc.Dimension   = ImageDim::ImageCubemap;
        desc.ImageSize   = { width, width, 1 };
        desc.ArraySize   = 6 * arraySize;
        desc.ImageFormat = format;
        return desc;
    }

    ImageDesc ImageDesc::Img3D(ImageBindFlags bindFlags, UInt32 width, UInt32 height, UInt32 depth, Format format)
    {
        ImageDesc desc{};
        desc.BindFlags   = bindFlags;
        desc.Dimension   = ImageDim::Image3D;
        desc.ImageSize   = { width, height, depth };
        desc.ArraySize   = 1;
        desc.ImageFormat = format;
        return desc;
    }
} // namespace FE::GPU
