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

        [[nodiscard]] Vector2UInt GetSize2D() const
        {
            return Vector2UInt{ m_width, m_height };
        }

        void SetSize(const Vector3UInt size)
        {
            m_width = size.x;
            m_height = size.y;
            m_depth = size.z;
        }

        void SetSize(const Vector2UInt size)
        {
            m_width = size.x;
            m_height = size.y;
            m_depth = 1;
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


    struct ImageSubresource final
    {
        uint32_t m_mostDetailedMipSlice : 4;
        uint32_t m_mipSliceCount : 4;
        uint32_t m_firstArraySlice : 11;
        uint32_t m_arraySize : 11;
        ImageAspect m_aspect : 2;

        static ImageSubresource CreateWhole(const ImageDesc imageDesc)
        {
            ImageSubresource subresource;
            subresource.m_mostDetailedMipSlice = 0;
            subresource.m_mipSliceCount = imageDesc.m_mipSliceCount;
            subresource.m_firstArraySlice = 0;
            subresource.m_arraySize = imageDesc.m_arraySize;
            subresource.m_aspect = FormatInfo{ imageDesc.m_imageFormat }.m_aspectFlags;
            return subresource;
        }

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

        explicit ImageSubresourceIterator(const ImageDesc imageDesc)
            : m_subresource(ImageSubresource::CreateWhole(imageDesc))
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
} // namespace FE::Graphics::Core
