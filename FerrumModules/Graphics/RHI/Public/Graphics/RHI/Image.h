#pragma once
#include <Graphics/RHI/ImageEnums.h>
#include <Graphics/RHI/ImageFormat.h>
#include <Graphics/RHI/ImageSubresource.h>
#include <Graphics/RHI/Resource.h>
#include <Graphics/RHI/ResourceState.h>

namespace FE::Graphics::RHI
{
    struct ImageDesc final
    {
        uint32_t m_width : 14;
        uint32_t m_height : 14;
        uint32_t m_sampleCount : 4;
        uint32_t m_depth : 14;
        uint32_t m_arraySize : 12;
        uint32_t m_mipSliceCount : 4;
        ImageDim m_dimension : 2;

        Format m_imageFormat = Format::kUndefined;

        ImageBindFlags m_bindFlags = ImageBindFlags::kShaderRead;

        ImageDesc()
        {
            m_width = 0;
            m_height = 0;
            m_depth = 0;
            m_arraySize = 1;
            m_mipSliceCount = 1;
            m_sampleCount = 1;
            m_dimension = ImageDim::kImage2D;
        }

        Size GetSize() const
        {
            return Size{ m_width, m_height, m_depth };
        }

        void SetSize(Size size)
        {
            m_width = size.width;
            m_height = size.height;
            m_depth = size.depth;
        }

        static ImageDesc Img1D(ImageBindFlags bindFlags, uint32_t width, Format format);
        static ImageDesc Img1DArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format);
        static ImageDesc Img2D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, Format format, bool useMipMaps = false,
                               uint32_t sampleCount = 1);
        static ImageDesc Img2DArray(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint16_t arraySize, Format format,
                                    bool useMipMaps = false);
        static ImageDesc ImgCubemap(ImageBindFlags bindFlags, uint32_t width, Format format);
        static ImageDesc ImgCubemapArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format);
        static ImageDesc Img3D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint32_t depth, Format format);
    };


    inline ImageDesc ImageDesc::Img1D(ImageBindFlags bindFlags, uint32_t width, Format format)
    {
        return ImageDesc::Img1DArray(bindFlags, width, 1, format);
    }


    inline ImageDesc ImageDesc::Img1DArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format)
    {
        ImageDesc desc{};
        desc.m_bindFlags = bindFlags;
        desc.m_dimension = ImageDim::kImage1D;
        desc.m_imageFormat = format;
        desc.m_arraySize = arraySize;
        desc.SetSize({ width, 1, 1 });
        return desc;
    }


    inline ImageDesc ImageDesc::Img2D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, Format format, bool useMipMaps,
                                      uint32_t sampleCount)
    {
        ImageDesc desc{};
        desc.m_bindFlags = bindFlags;
        desc.m_dimension = ImageDim::kImage2D;
        desc.m_arraySize = 1;
        desc.m_imageFormat = format;
        desc.m_sampleCount = sampleCount;
        desc.m_mipSliceCount = useMipMaps ? CalculateMipCount({ width, height }) : 1;
        desc.SetSize({ width, height, 1 });
        return desc;
    }


    inline ImageDesc ImageDesc::Img2DArray(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint16_t arraySize,
                                           Format format, bool useMipMaps)
    {
        ImageDesc desc{};
        desc.m_bindFlags = bindFlags;
        desc.m_dimension = ImageDim::kImage2D;
        desc.m_arraySize = arraySize;
        desc.m_imageFormat = format;
        desc.m_mipSliceCount = useMipMaps ? CalculateMipCount({ width, height }) : 1;
        desc.SetSize({ width, height, 1 });
        return desc;
    }


    inline ImageDesc ImageDesc::ImgCubemap(ImageBindFlags bindFlags, uint32_t width, Format format)
    {
        return ImageDesc::ImgCubemapArray(bindFlags, width, 1, format);
    }


    inline ImageDesc ImageDesc::ImgCubemapArray(ImageBindFlags bindFlags, uint32_t width, uint16_t arraySize, Format format)
    {
        ImageDesc desc;
        desc.m_bindFlags = bindFlags;
        desc.m_dimension = ImageDim::kImageCubemap;
        desc.m_arraySize = 6 * arraySize;
        desc.m_imageFormat = format;
        desc.SetSize({ width, width, 1 });
        return desc;
    }


    inline ImageDesc ImageDesc::Img3D(ImageBindFlags bindFlags, uint32_t width, uint32_t height, uint32_t depth, Format format)
    {
        ImageDesc desc{};
        desc.m_bindFlags = bindFlags;
        desc.m_dimension = ImageDim::kImage3D;
        desc.m_arraySize = 1;
        desc.m_imageFormat = format;
        desc.SetSize({ width, height, depth });
        return desc;
    }


    struct ImageView;
    struct DeviceMemory;

    struct Image : public Resource
    {
        FE_RTTI_Class(Image, "4C4B8F44-E965-479D-B12B-264C9BF63A49");

        ~Image() override = default;

        virtual ResultCode Init(StringSlice name, const ImageDesc& desc) = 0;

        virtual const ImageDesc& GetDesc() = 0;

        virtual void SetState(const ImageSubresourceRange& subresourceRange, ResourceState state) = 0;
        [[nodiscard]] virtual ResourceState GetState(const ImageSubresourceRange& subresourceRange) const = 0;
        [[nodiscard]] virtual ResourceState GetState(uint16_t arraySlice, uint16_t mipSlice) const = 0;
    };
} // namespace FE::Graphics::RHI


template<>
struct eastl::hash<FE::Graphics::RHI::ImageDesc>
{
    size_t operator()(const FE::Graphics::RHI::ImageDesc& imageDesc) const
    {
        return FE::DefaultHash(&imageDesc, sizeof(FE::Graphics::RHI::ImageDesc));
    }
};
