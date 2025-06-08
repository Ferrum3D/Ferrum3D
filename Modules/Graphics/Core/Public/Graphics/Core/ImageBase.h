#pragma once
#include <Graphics/Core/ImageFormat.h>

namespace FE::Graphics::Core
{
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

        static const ImageSubresource kInvalid;

        friend bool operator==(const ImageSubresource& lhs, const ImageSubresource& rhs)
        {
            return festd::bit_cast<uint32_t>(lhs) == festd::bit_cast<uint32_t>(rhs);
        }

        friend bool operator!=(const ImageSubresource& lhs, const ImageSubresource& rhs)
        {
            return festd::bit_cast<uint32_t>(lhs) != festd::bit_cast<uint32_t>(rhs);
        }
    };

    inline const ImageSubresource ImageSubresource::kInvalid = festd::bit_cast<ImageSubresource>(kInvalidIndex);


    struct ImageSubresourceIterator final
    {
        struct Slice final
        {
            int32_t m_mipIndex : 16;
            int32_t m_arrayIndex : 16;
        };

        struct Iter final
        {
            ImageSubresource m_subresource;
            Slice m_currentSlice;

            Iter operator++()
            {
                ++m_currentSlice.m_mipIndex;

                const uint32_t lastMipSlice = m_subresource.m_mostDetailedMipSlice + m_subresource.m_mipSliceCount;
                if (m_currentSlice.m_mipIndex == static_cast<int32_t>(lastMipSlice))
                {
                    ++m_currentSlice.m_arrayIndex;
                    m_currentSlice.m_mipIndex = static_cast<int32_t>(m_subresource.m_mostDetailedMipSlice);
                }

                return *this;
            }

            Iter operator++(int)
            {
                const Iter iter = *this;
                ++*this;
                return iter;
            }

            [[nodiscard]] Slice operator*() const
            {
                return m_currentSlice;
            }

            [[nodiscard]] bool operator==(const Iter other) const
            {
                return festd::bit_cast<uint32_t>(m_currentSlice) == festd::bit_cast<uint32_t>(other.m_currentSlice);
            }

            [[nodiscard]] bool operator!=(const Iter other) const
            {
                return festd::bit_cast<uint32_t>(m_currentSlice) != festd::bit_cast<uint32_t>(other.m_currentSlice);
            }
        };

        ImageSubresource m_subresource;

        explicit ImageSubresourceIterator(const ImageSubresource subresource)
            : m_subresource(subresource)
        {
        }

        [[nodiscard]] Iter begin() const
        {
            Iter iter;
            iter.m_subresource = m_subresource;
            iter.m_currentSlice.m_mipIndex = static_cast<int32_t>(m_subresource.m_mostDetailedMipSlice);
            iter.m_currentSlice.m_arrayIndex = static_cast<int32_t>(m_subresource.m_firstArraySlice);
            return iter;
        }

        [[nodiscard]] Iter end() const
        {
            Iter iter;
            iter.m_subresource = m_subresource;
            iter.m_currentSlice.m_mipIndex = static_cast<int32_t>(m_subresource.m_mostDetailedMipSlice);
            iter.m_currentSlice.m_arrayIndex = static_cast<int32_t>(m_subresource.m_firstArraySlice + m_subresource.m_arraySize);
            return iter;
        }
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
        Format m_imageFormat;

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

        static ImageDesc Img1D(uint32_t width, Format format);
        static ImageDesc Img1DArray(uint32_t width, uint16_t arraySize, Format format);
        static ImageDesc Img2D(uint32_t width, uint32_t height, Format format, bool useMipMaps = false, uint32_t sampleCount = 1);
        static ImageDesc Img2DArray(uint32_t width, uint32_t height, uint16_t arraySize, Format format, bool useMipMaps = false);
        static ImageDesc ImgCubemap(uint32_t width, Format format);
        static ImageDesc ImgCubemapArray(uint32_t width, uint16_t arraySize, Format format);
        static ImageDesc Img3D(uint32_t width, uint32_t height, uint32_t depth, Format format);
    };


    inline ImageDesc ImageDesc::Img1D(const uint32_t width, const Format format)
    {
        return Img1DArray(width, 1, format);
    }


    inline ImageDesc ImageDesc::Img1DArray(const uint32_t width, const uint16_t arraySize, const Format format)
    {
        ImageDesc desc{};
        desc.m_dimension = ImageDimension::k1D;
        desc.m_imageFormat = format;
        desc.m_arraySize = arraySize;
        desc.SetSize({ width, 1, 1 });
        return desc;
    }


    inline ImageDesc ImageDesc::Img2D(const uint32_t width, const uint32_t height, const Format format, const bool useMipMaps,
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


    inline ImageDesc ImageDesc::Img2DArray(const uint32_t width, const uint32_t height, const uint16_t arraySize,
                                           const Format format, const bool useMipMaps)
    {
        ImageDesc desc{};
        desc.m_dimension = ImageDimension::k2D;
        desc.m_arraySize = arraySize;
        desc.m_imageFormat = format;
        desc.m_mipSliceCount = useMipMaps ? CalculateMipCount({ width, height, 1 }) : 1;
        desc.SetSize({ width, height, 1 });
        return desc;
    }


    inline ImageDesc ImageDesc::ImgCubemap(const uint32_t width, const Format format)
    {
        return ImgCubemapArray(width, 1, format);
    }


    inline ImageDesc ImageDesc::ImgCubemapArray(const uint32_t width, const uint16_t arraySize, const Format format)
    {
        ImageDesc desc{};
        desc.m_dimension = ImageDimension::kCubemap;
        desc.m_arraySize = 6 * arraySize;
        desc.m_imageFormat = format;
        desc.SetSize({ width, width, 1 });
        return desc;
    }


    inline ImageDesc ImageDesc::Img3D(const uint32_t width, const uint32_t height, const uint32_t depth, const Format format)
    {
        ImageDesc desc{};
        desc.m_dimension = ImageDimension::k3D;
        desc.m_arraySize = 1;
        desc.m_imageFormat = format;
        desc.SetSize({ width, height, depth });
        return desc;
    }
} // namespace FE::Graphics::Core
