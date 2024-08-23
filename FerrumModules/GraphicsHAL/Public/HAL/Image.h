#pragma once
#include <HAL/BindFlags.h>
#include <HAL/ImageEnums.h>
#include <HAL/ImageFormat.h>
#include <HAL/ImageSubresource.h>
#include <HAL/Resource.h>
#include <HAL/ResourceState.h>

namespace FE::Graphics::HAL
{
    struct ImageDesc
    {
        Size ImageSize = {};

        Format ImageFormat = Format::None;
        ImageDim Dimension = ImageDim::Image2D;

        ImageBindFlags BindFlags = ImageBindFlags::ShaderRead;

        uint32_t MipSliceCount = 1;
        uint32_t SampleCount = 1;
        uint16_t ArraySize = 1;

        inline static ImageDesc Img1D(ImageBindFlags bindFlags, uint32_t width, Format format);
        inline static ImageDesc Img1DArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format);
        inline static ImageDesc Img2D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, Format format,
                                      bool useMipMaps = false, uint32_t sampleCount = 1);
        inline static ImageDesc Img2DArray(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint16_t arraySize,
                                           Format format, bool useMipMaps = false);
        inline static ImageDesc ImgCubemap(ImageBindFlags bindFlags, uint32_t width, Format format);
        inline static ImageDesc ImgCubemapArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format);
        inline static ImageDesc Img3D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint32_t depth, Format format);
    };

    ImageDesc ImageDesc::Img1D(ImageBindFlags bindFlags, uint32_t width, Format format)
    {
        return ImageDesc::Img1DArray(bindFlags, width, 1, format);
    }

    ImageDesc ImageDesc::Img1DArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format)
    {
        ImageDesc desc{};
        desc.BindFlags = bindFlags;
        desc.Dimension = ImageDim::Image1D;
        desc.ImageFormat = format;
        desc.ImageSize = { width, 1, 1 };
        desc.ArraySize = arraySize;
        return desc;
    }

    ImageDesc ImageDesc::Img2D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, Format format, bool useMipMaps,
                               uint32_t sampleCount)
    {
        ImageDesc desc{};
        desc.BindFlags = bindFlags;
        desc.Dimension = ImageDim::Image2D;
        desc.ImageSize = { width, height, 1 };
        desc.ArraySize = 1;
        desc.ImageFormat = format;
        desc.SampleCount = sampleCount;
        desc.MipSliceCount = useMipMaps ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 : 1;
        return desc;
    }

    ImageDesc ImageDesc::Img2DArray(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint16_t arraySize, Format format,
                                    bool useMipMaps)
    {
        ImageDesc desc{};
        desc.BindFlags = bindFlags;
        desc.Dimension = ImageDim::Image2D;
        desc.ImageSize = { width, height, 1 };
        desc.ArraySize = arraySize;
        desc.ImageFormat = format;
        desc.MipSliceCount = useMipMaps ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1 : 1;
        return desc;
    }

    ImageDesc ImageDesc::ImgCubemap(ImageBindFlags bindFlags, uint32_t width, Format format)
    {
        return ImageDesc::ImgCubemapArray(bindFlags, width, 1, format);
    }

    ImageDesc ImageDesc::ImgCubemapArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format)
    {
        ImageDesc desc;
        desc.BindFlags = bindFlags;
        desc.Dimension = ImageDim::ImageCubemap;
        desc.ImageSize = { width, width, 1 };
        desc.ArraySize = 6 * arraySize;
        desc.ImageFormat = format;
        return desc;
    }

    ImageDesc ImageDesc::Img3D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint32_t depth, Format format)
    {
        ImageDesc desc{};
        desc.BindFlags = bindFlags;
        desc.Dimension = ImageDim::Image3D;
        desc.ImageSize = { width, height, depth };
        desc.ArraySize = 1;
        desc.ImageFormat = format;
        return desc;
    }

    class ImageView;
    class DeviceMemory;

    class Image : public Resource
    {
    public:
        FE_RTTI_Class(Image, "4C4B8F44-E965-479D-B12B-264C9BF63A49");

        ~Image() override = default;

        virtual ResultCode Init(StringSlice name, const ImageDesc& desc) = 0;

        virtual const ImageDesc& GetDesc() = 0;

        virtual void SetState(const ImageSubresourceRange& subresourceRange, ResourceState state) = 0;
        [[nodiscard]] virtual ResourceState GetState(const ImageSubresourceRange& subresourceRange) const = 0;
        [[nodiscard]] virtual ResourceState GetState(uint16_t arraySlice, uint16_t mipSlice) const = 0;
    };
} // namespace FE::Graphics::HAL

FE_MAKE_HASHABLE(FE::Graphics::HAL::ImageDesc, , value.ImageSize, value.ImageFormat, value.Dimension, value.BindFlags,
                 value.MipSliceCount, value.SampleCount, value.ArraySize);
