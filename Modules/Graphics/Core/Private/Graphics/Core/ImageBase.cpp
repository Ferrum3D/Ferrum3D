#include <Graphics/Core/ImageBase.h>

namespace FE::Graphics::Core
{
    ImageDesc ImageDesc::Img1D(const uint32_t width, const Format format)
    {
        return Img1DArray(width, 1, format);
    }


    ImageDesc ImageDesc::Img1DArray(const uint32_t width, const uint16_t arraySize, const Format format)
    {
        ImageDesc desc{};
        desc.m_dimension = ImageDimension::k1D;
        desc.m_imageFormat = format;
        desc.m_arraySize = arraySize;
        desc.SetSize({ width, 1, 1 });
        return desc;
    }


    ImageDesc ImageDesc::Img2D(const uint32_t width, const uint32_t height, const Format format, const bool useMipMaps,
                               const uint32_t sampleCount)
    {
        ImageDesc desc{};
        desc.m_dimension = ImageDimension::k2D;
        desc.m_arraySize = 1;
        desc.m_imageFormat = format;
        desc.m_sampleCount = sampleCount;
        desc.m_mipSliceCount = useMipMaps ? CalculateMipCount({ width, height, 1 }) : 1;
        desc.SetSize({ width, height, 1 });
        return desc;
    }


    ImageDesc ImageDesc::Img2DArray(const uint32_t width, const uint32_t height, const uint16_t arraySize, const Format format,
                                    const bool useMipMaps)
    {
        ImageDesc desc{};
        desc.m_dimension = ImageDimension::k2D;
        desc.m_arraySize = arraySize;
        desc.m_imageFormat = format;
        desc.m_mipSliceCount = useMipMaps ? CalculateMipCount({ width, height, 1 }) : 1;
        desc.SetSize({ width, height, 1 });
        return desc;
    }


    ImageDesc ImageDesc::ImgCubemap(const uint32_t width, const Format format)
    {
        return ImgCubemapArray(width, 1, format);
    }


    ImageDesc ImageDesc::ImgCubemapArray(const uint32_t width, const uint16_t arraySize, const Format format)
    {
        ImageDesc desc{};
        desc.m_dimension = ImageDimension::kCubemap;
        desc.m_arraySize = 6 * arraySize;
        desc.m_imageFormat = format;
        desc.SetSize({ width, width, 1 });
        return desc;
    }


    ImageDesc ImageDesc::Img3D(const uint32_t width, const uint32_t height, const uint32_t depth, const Format format)
    {
        ImageDesc desc{};
        desc.m_dimension = ImageDimension::k3D;
        desc.m_arraySize = 1;
        desc.m_imageFormat = format;
        desc.SetSize({ width, height, depth });
        return desc;
    }
} // namespace FE::Graphics::Core
