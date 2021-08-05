#include <FeGPU/Image/IImage.h>

namespace FE::GPU
{
    ImageDesc ImageDesc::Img1D(ImageBindFlags bindFlags, uint32_t width, Format format)
    {
        return ImageDesc::Img1DArray(bindFlags, width, 1, format);
    }

    ImageDesc ImageDesc::Img1DArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format)
    {
        ImageDesc desc{};
        desc.BindFlags   = bindFlags;
        desc.Dimention   = ImageDim::Image1D;
        desc.ImageFormat = format;
        desc.ImageSize   = { width, 1, 1 };
        desc.ArraySize   = arraySize;
        return desc;
    }

    ImageDesc ImageDesc::Img2D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, Format format)
    {
        return ImageDesc::Img2DArray(bindFlags, width, height, 1, format);
    }

    ImageDesc ImageDesc::Img2DArray(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint16_t arraySize, Format format)
    {
        ImageDesc desc{};
        desc.BindFlags   = bindFlags;
        desc.Dimention   = ImageDim::Image2D;
        desc.ImageSize   = { width, height, 1 };
        desc.ArraySize   = arraySize;
        desc.ImageFormat = format;
        return desc;
    }

    ImageDesc ImageDesc::ImgCubemap(ImageBindFlags bindFlags, uint32_t width, Format format)
    {
        return ImageDesc::ImgCubemapArray(bindFlags, width, 1, format);
    }

    ImageDesc ImageDesc::ImgCubemapArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format)
    {
        ImageDesc desc;
        desc.BindFlags   = bindFlags;
        desc.Dimention   = ImageDim::ImageCubemap;
        desc.ImageSize   = { width, width, 1 };
        desc.ArraySize   = 6 * arraySize;
        desc.ImageFormat = format;
        return desc;
    }

    ImageDesc ImageDesc::Img3D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint32_t depth, Format format)
    {
        ImageDesc desc{};
        desc.BindFlags   = bindFlags;
        desc.Dimention   = ImageDim::Image3D;
        desc.ImageSize   = { width, height, depth };
        desc.ArraySize   = 1;
        desc.ImageFormat = format;
        return desc;
    }
} // namespace FE::GPU
