#pragma once
#include <FeCore/Memory/Object.h>
#include <GPU/Common/BaseTypes.h>
#include <GPU/Image/ImageEnums.h>
#include <GPU/Image/ImageFormat.h>
#include <GPU/Resource/IResource.h>
#include <cstdint>

namespace FE::GPU
{
    struct ImageDesc
    {
        Size ImageSize = { 0, 0, 0 };

        Format ImageFormat = Format::None;
        ImageDim Dimension = ImageDim::Image2D;

        ImageBindFlags BindFlags = ImageBindFlags::ShaderRead;

        UInt16 ArraySize     = 1;
        UInt32 MipLevelCount = 1;
        UInt32 SampleCount   = 1;

        FE_STRUCT_RTTI(ImageDesc, "1B7CB069-C763-49D0-9CEA-088681802761");

        static ImageDesc Img1D(ImageBindFlags bindFlags, UInt32 width, Format format);
        static ImageDesc Img1DArray(ImageBindFlags bindFlags, UInt32 width, UInt16 arraySize, Format format);
        static ImageDesc Img2D(ImageBindFlags bindFlags, UInt32 width, UInt32 height, Format format);
        static ImageDesc Img2DArray(ImageBindFlags bindFlags, UInt32 width, UInt32 height, UInt16 arraySize, Format format);
        static ImageDesc ImgCubemap(ImageBindFlags bindFlags, UInt32 width, Format format);
        static ImageDesc ImgCubemapArray(ImageBindFlags bindFlags, UInt32 width, UInt16 arraySize, Format format);
        static ImageDesc Img3D(ImageBindFlags bindFlags, UInt32 width, UInt32 height, UInt32 depth, Format format);
    };

    class IImageView;

    class IImage : public IObject
    {
    public:
        FE_CLASS_RTTI(IImage, "4C4B8F44-E965-479D-B12B-264C9BF63A49");

        ~IImage() override = default;

        virtual const ImageDesc& GetDesc()                  = 0;
        virtual Shared<IImageView> CreateRenderTargetView() = 0;
    };
} // namespace FE::GPU
