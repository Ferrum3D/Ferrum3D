#pragma once
#include <HAL/BindFlags.h>
#include <HAL/ImageEnums.h>
#include <HAL/ImageFormat.h>
#include <HAL/ImageSubresource.h>
#include <HAL/Resource.h>
#include <HAL/ResourceState.h>

namespace FE::Graphics::HAL
{
    struct ImageDesc final
    {
        uint32_t Width : 14;
        uint32_t Height : 14;
        uint32_t SampleCount : 4;
        uint32_t Depth : 14;
        uint32_t ArraySize : 12;
        uint32_t MipSliceCount : 4;
        ImageDim Dimension : 2;

        Format ImageFormat = Format::kUndefined;

        ImageBindFlags BindFlags = ImageBindFlags::kShaderRead;

        inline ImageDesc()
        {
            Width = 0;
            Height = 0;
            Depth = 0;
            ArraySize = 1;
            MipSliceCount = 1;
            SampleCount = 1;
            Dimension = ImageDim::kImage2D;
        }

        inline Size GetSize() const
        {
            return Size{ Width, Height, Depth };
        }

        inline void SetSize(Size size)
        {
            Width = size.Width;
            Height = size.Height;
            Depth = size.Depth;
        }

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
        desc.Dimension = ImageDim::kImage1D;
        desc.ImageFormat = format;
        desc.SetSize({ width, 1, 1 });
        desc.ArraySize = arraySize;
        return desc;
    }

    ImageDesc ImageDesc::Img2D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, Format format, bool useMipMaps,
                               uint32_t sampleCount)
    {
        ImageDesc desc{};
        desc.BindFlags = bindFlags;
        desc.Dimension = ImageDim::kImage2D;
        desc.SetSize({ width, height, 1 });
        desc.ArraySize = 1;
        desc.ImageFormat = format;
        desc.SampleCount = sampleCount;
        desc.MipSliceCount = useMipMaps ? CalculateMipCount({ width, height }) : 1;
        return desc;
    }

    ImageDesc ImageDesc::Img2DArray(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint16_t arraySize, Format format,
                                    bool useMipMaps)
    {
        ImageDesc desc{};
        desc.BindFlags = bindFlags;
        desc.Dimension = ImageDim::kImage2D;
        desc.SetSize({ width, height, 1 });
        desc.ArraySize = arraySize;
        desc.ImageFormat = format;
        desc.MipSliceCount = useMipMaps ? CalculateMipCount({ width, height }) : 1;
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
        desc.Dimension = ImageDim::kImageCubemap;
        desc.SetSize({ width, width, 1 });
        desc.ArraySize = 6 * arraySize;
        desc.ImageFormat = format;
        return desc;
    }

    ImageDesc ImageDesc::Img3D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint32_t depth, Format format)
    {
        ImageDesc desc{};
        desc.BindFlags = bindFlags;
        desc.Dimension = ImageDim::kImage3D;
        desc.SetSize({ width, height, depth });
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


template<>
struct eastl::hash<FE::Graphics::HAL::ImageDesc>
{
    size_t operator()(const FE::Graphics::HAL::ImageDesc& imageDesc) const
    {
        return FE::DefaultHash(&imageDesc, sizeof(FE::Graphics::HAL::ImageDesc));
    }
};
