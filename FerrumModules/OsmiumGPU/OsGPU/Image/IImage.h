#pragma once
#include <FeCore/Memory/SharedPtr.h>
#include <OsGPU/Common/BaseTypes.h>
#include <OsGPU/Image/ImageEnums.h>
#include <OsGPU/Image/ImageFormat.h>
#include <OsGPU/Memory/MemoryType.h>
#include <OsGPU/Resource/BindFlags.h>
#include <cstdint>

namespace FE::Osmium
{
    struct ImageDesc
    {
        Size ImageSize = {};

        Format ImageFormat = Format::None;
        ImageDim Dimension = ImageDim::Image2D;

        ImageBindFlags BindFlags = ImageBindFlags::ShaderRead;

        UInt32 MipLevelCount = 1;
        UInt32 SampleCount   = 1;
        UInt16 ArraySize     = 1;

        FE_STRUCT_RTTI(ImageDesc, "1B7CB069-C763-49D0-9CEA-088681802761");

        inline static ImageDesc Img1D(ImageBindFlags bindFlags, UInt32 width, Format format);
        inline static ImageDesc Img1DArray(ImageBindFlags bindFlags, UInt32 width, UInt16 arraySize, Format format);
        inline static ImageDesc Img2D(ImageBindFlags bindFlags, UInt32 width, UInt32 height, Format format);
        inline static ImageDesc Img2DArray(
            ImageBindFlags bindFlags, UInt32 width, UInt32 height, UInt16 arraySize, Format format);
        inline static ImageDesc ImgCubemap(ImageBindFlags bindFlags, UInt32 width, Format format);
        inline static ImageDesc ImgCubemapArray(ImageBindFlags bindFlags, UInt32 width, UInt16 arraySize, Format format);
        inline static ImageDesc Img3D(ImageBindFlags bindFlags, UInt32 width, UInt32 height, UInt32 depth, Format format);
    };

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

    class IImageView;
    class IDeviceMemory;

    class IImage : public IObject
    {
    public:
        FE_CLASS_RTTI(IImage, "4C4B8F44-E965-479D-B12B-264C9BF63A49");

        ~IImage() override = default;

        virtual const ImageDesc& GetDesc()      = 0;
        virtual Shared<IImageView> CreateView() = 0;

        virtual void AllocateMemory(MemoryType type)                                = 0;
        virtual void BindMemory(const Shared<IDeviceMemory>& memory, UInt64 offset) = 0;
    };
} // namespace FE::Osmium
