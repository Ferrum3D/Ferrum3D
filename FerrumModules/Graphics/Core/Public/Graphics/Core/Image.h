#pragma once
#include <FeCore/Math/Vector3UInt.h>
#include <Graphics/Core/ImageFormat.h>
#include <Graphics/Core/Resource.h>

namespace FE::Graphics::Core
{
    enum class ImageBindFlags : uint32_t
    {
        kNone = 0,

        kShaderRead = 1 << 0,
        kUnorderedAccess = 1 << 1,

        kColorTarget = 1 << 2,
        kDepthStencilTarget = 1 << 3,

        kTransferSrc = 1 << 4,
        kTransferDst = 1 << 5,
    };

    FE_ENUM_OPERATORS(ImageBindFlags);


    enum class ImageDimension : uint32_t
    {
        k1D,
        k2D,
        k3D,
        kCubemap,
    };


    struct ImageSubresource final
    {
        uint32_t m_mostDetailedMipSlice : 4;
        uint32_t m_mipSliceCount : 4;
        uint32_t m_firstArraySlice : 12;
        uint32_t m_arraySize : 12;
    };


    struct ImageDesc final
    {
        uint32_t m_width : 14;
        uint32_t m_height : 14;
        uint32_t m_sampleCount : 4;
        uint32_t m_depth : 14;
        uint32_t m_arraySize : 12;
        uint32_t m_mipSliceCount : 4;
        ImageDimension m_dimension : 2;
        ImageBindFlags m_bindFlags : 16;
        ResourceUsage m_usage : 16;

        Format m_imageFormat = Format::kUndefined;

        ImageDesc()
        {
            m_width = 0;
            m_height = 0;
            m_depth = 0;
            m_arraySize = 1;
            m_mipSliceCount = 1;
            m_sampleCount = 1;
            m_dimension = ImageDimension::k2D;
            m_bindFlags = ImageBindFlags::kShaderRead;
            m_usage = ResourceUsage::kDeviceOnly;
        }

        [[nodiscard]] Vector3UInt GetSize() const
        {
            return Vector3UInt{ m_width, m_height, m_depth };
        }

        void SetSize(const Vector3UInt size)
        {
            m_width = size.x;
            m_height = size.y;
            m_depth = size.z;
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            return DefaultHash(this, sizeof(*this));
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


    inline ImageDesc ImageDesc::Img1D(const ImageBindFlags bindFlags, const uint32_t width, const Format format)
    {
        return Img1DArray(bindFlags, width, 1, format);
    }


    inline ImageDesc ImageDesc::Img1DArray(const ImageBindFlags bindFlags, const uint32_t width, const uint16_t arraySize,
                                           const Format format)
    {
        ImageDesc desc{};
        desc.m_bindFlags = bindFlags;
        desc.m_dimension = ImageDimension::k1D;
        desc.m_imageFormat = format;
        desc.m_arraySize = arraySize;
        desc.SetSize({ width, 1, 1 });
        return desc;
    }


    inline ImageDesc ImageDesc::Img2D(const ImageBindFlags bindFlags, const uint32_t width, const uint32_t height,
                                      const Format format, const bool useMipMaps, const uint32_t sampleCount)
    {
        ImageDesc desc{};
        desc.m_bindFlags = bindFlags;
        desc.m_dimension = ImageDimension::k2D;
        desc.m_arraySize = 1;
        desc.m_imageFormat = format;
        desc.m_sampleCount = sampleCount;
        desc.m_mipSliceCount = useMipMaps ? CalculateMipCount({ width, height, 1 }) : 1;
        desc.SetSize({ width, height, 1 });
        return desc;
    }


    inline ImageDesc ImageDesc::Img2DArray(const ImageBindFlags bindFlags, const uint32_t width, const uint32_t height,
                                           const uint16_t arraySize, const Format format, const bool useMipMaps)
    {
        ImageDesc desc{};
        desc.m_bindFlags = bindFlags;
        desc.m_dimension = ImageDimension::k2D;
        desc.m_arraySize = arraySize;
        desc.m_imageFormat = format;
        desc.m_mipSliceCount = useMipMaps ? CalculateMipCount({ width, height, 1 }) : 1;
        desc.SetSize({ width, height, 1 });
        return desc;
    }


    inline ImageDesc ImageDesc::ImgCubemap(const ImageBindFlags bindFlags, const uint32_t width, const Format format)
    {
        return ImgCubemapArray(bindFlags, width, 1, format);
    }


    inline ImageDesc ImageDesc::ImgCubemapArray(const ImageBindFlags bindFlags, const uint32_t width, const uint16_t arraySize,
                                                const Format format)
    {
        ImageDesc desc;
        desc.m_bindFlags = bindFlags;
        desc.m_dimension = ImageDimension::kCubemap;
        desc.m_arraySize = 6 * arraySize;
        desc.m_imageFormat = format;
        desc.SetSize({ width, width, 1 });
        return desc;
    }


    inline ImageDesc ImageDesc::Img3D(const ImageBindFlags bindFlags, const uint32_t width, const uint32_t height,
                                      const uint32_t depth, const Format format)
    {
        ImageDesc desc{};
        desc.m_bindFlags = bindFlags;
        desc.m_dimension = ImageDimension::k3D;
        desc.m_arraySize = 1;
        desc.m_imageFormat = format;
        desc.SetSize({ width, height, depth });
        return desc;
    }


    struct Image : public Resource
    {
        FE_RTTI_Class(Image, "4C4B8F44-E965-479D-B12B-264C9BF63A49");

        virtual const ImageDesc& GetDesc() const = 0;
    };
} // namespace FE::Graphics::Core


template<>
struct eastl::hash<FE::Graphics::Core::ImageDesc>
{
    size_t operator()(const FE::Graphics::Core::ImageDesc& imageDesc) const
    {
        return imageDesc.GetHash();
    }
};
