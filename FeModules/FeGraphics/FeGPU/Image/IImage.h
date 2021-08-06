#pragma once
#include <FeGPU/Common/BaseTypes.h>
#include <FeGPU/Image/ImageFormat.h>
#include <FeGPU/Resource/IResource.h>
#include <FeGPU/Image/ImageEnums.h>
#include <cstdint>

namespace FE::GPU
{
    struct ImageDesc
    {
        Size ImageSize = { 0, 0, 0 };

        Format ImageFormat = Format::None;
        ImageDim Dimention = ImageDim::Image2D;

        ImageBindFlags BindFlags = ImageBindFlags::ShaderRead;

        uint32_t ArraySize     = 1;
        uint32_t MipLevelCount = 1;
        uint32_t SampleCount   = 1;

        static ImageDesc Img1D(ImageBindFlags bindFlags, uint32_t width, Format format);
        static ImageDesc Img1DArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format);
        static ImageDesc Img2D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, Format format);
        static ImageDesc Img2DArray(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint16_t arraySize, Format format);
        static ImageDesc ImgCubemap(ImageBindFlags bindFlags, uint32_t width, Format format);
        static ImageDesc ImgCubemapArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format);
        static ImageDesc Img3D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint32_t depth, Format format);
    };

    class IImage
    {
    public:
        virtual ~IImage() = default;

        virtual const ImageDesc& GetDesc() = 0;
    };
} // namespace FE::GPU
