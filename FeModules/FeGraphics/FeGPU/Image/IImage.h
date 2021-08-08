#pragma once
#include <FeGPU/Common/BaseTypes.h>
#include <FeGPU/Image/ImageFormat.h>
#include <FeGPU/Resource/IResource.h>
#include <FeGPU/Image/ImageEnums.h>
#include <FeCore/Memory/Object.h>
#include <cstdint>

namespace FE::GPU
{
    struct ImageDesc
    {
        Size ImageSize = { 0, 0, 0 };

        Format ImageFormat = Format::None;
        ImageDim Dimention = ImageDim::Image2D;

        ImageBindFlags BindFlags = ImageBindFlags::ShaderRead;

        UInt32 ArraySize     = 1;
        UInt32 MipLevelCount = 1;
        UInt32 SampleCount   = 1;

        static ImageDesc Img1D(ImageBindFlags bindFlags, UInt32 width, Format format);
        static ImageDesc Img1DArray(ImageBindFlags bindFlags, UInt32 width, UInt16 arraySize, Format format);
        static ImageDesc Img2D(ImageBindFlags bindFlags, UInt32 width, UInt32 height, Format format);
        static ImageDesc Img2DArray(ImageBindFlags bindFlags, UInt32 width, UInt32 height, UInt16 arraySize, Format format);
        static ImageDesc ImgCubemap(ImageBindFlags bindFlags, UInt32 width, Format format);
        static ImageDesc ImgCubemapArray(ImageBindFlags bindFlags, UInt32 width, UInt16 arraySize, Format format);
        static ImageDesc Img3D(ImageBindFlags bindFlags, UInt32 width, UInt32 height, UInt32 depth, Format format);
    };

    class IImage : public IObject
    {
    public:
        virtual ~IImage() = default;

        virtual const ImageDesc& GetDesc() = 0;
    };
} // namespace FE::GPU
