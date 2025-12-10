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


    struct TextureDesc final
    {
        uint32_t m_width : 14;
        uint32_t m_height : 14;
        uint32_t m_sampleCount : 4;
        uint32_t m_depth : 14;
        uint32_t m_arraySize : 12;
        uint32_t m_mipSliceCount : 4;
        ImageDimension m_dimension : 2;
        Format m_imageFormat;

        TextureDesc()
        {
            memset(this, 0, sizeof(*this));
        }

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

        friend bool operator==(const TextureDesc lhs, const TextureDesc rhs)
        {
            return memcmp(&lhs, &rhs, sizeof(TextureDesc)) == 0;
        }

        friend bool operator!=(const TextureDesc lhs, const TextureDesc rhs)
        {
            return !(lhs == rhs);
        }
    };


    struct TextureSubresource final
    {
        uint32_t m_mostDetailedMipSlice : 4;
        uint32_t m_mipSliceCount : 4;
        uint32_t m_firstArraySlice : 11;
        uint32_t m_arraySize : 11;
        ImageAspect m_aspect : 2;

        static TextureSubresource CreateWhole(const TextureDesc desc)
        {
            TextureSubresource subresource;
            subresource.m_mostDetailedMipSlice = 0;
            subresource.m_mipSliceCount = desc.m_mipSliceCount;
            subresource.m_firstArraySlice = 0;
            subresource.m_arraySize = desc.m_arraySize;
            subresource.m_aspect = FormatInfo{ desc.m_imageFormat }.m_aspectFlags;
            return subresource;
        }

        static TextureSubresource Create(const TextureDesc desc, const uint32_t mipIndex, const uint32_t arrayIndex)
        {
            FE_Assert(mipIndex < desc.m_mipSliceCount);
            FE_Assert(arrayIndex < desc.m_arraySize);

            TextureSubresource subresource;
            subresource.m_mostDetailedMipSlice = mipIndex;
            subresource.m_mipSliceCount = 1;
            subresource.m_firstArraySlice = arrayIndex;
            subresource.m_arraySize = 1;
            subresource.m_aspect = FormatInfo{ desc.m_imageFormat }.m_aspectFlags;
            return subresource;
        }

        static const TextureSubresource kInvalid;

        [[nodiscard]] bool Contains(const TextureSubresource other) const
        {
            if (m_aspect != other.m_aspect)
                return false;

            return m_mostDetailedMipSlice <= other.m_mostDetailedMipSlice
                && m_mostDetailedMipSlice + m_mipSliceCount >= other.m_mostDetailedMipSlice + other.m_mipSliceCount
                && m_firstArraySlice <= other.m_firstArraySlice
                && m_firstArraySlice + m_arraySize >= other.m_firstArraySlice + other.m_arraySize;
        }

        [[nodiscard]] TextureSubresource Coalesce(const TextureSubresource other) const
        {
            FE_AssertDebug(m_aspect == other.m_aspect);

            TextureSubresource subresource;
            subresource.m_mostDetailedMipSlice = Math::Min(m_mostDetailedMipSlice, other.m_mostDetailedMipSlice);
            subresource.m_mipSliceCount =
                Math::Max(m_mostDetailedMipSlice + m_mipSliceCount, other.m_mostDetailedMipSlice + other.m_mipSliceCount)
                - subresource.m_mostDetailedMipSlice;
            subresource.m_firstArraySlice = Math::Min(m_firstArraySlice, other.m_firstArraySlice);
            subresource.m_arraySize = Math::Max(m_firstArraySlice + m_arraySize, other.m_firstArraySlice + other.m_arraySize)
                - subresource.m_firstArraySlice;
            subresource.m_aspect = m_aspect;
            return subresource;
        }

        friend bool operator<(const TextureSubresource lhs, const TextureSubresource rhs)
        {
            return festd::bit_cast<uint32_t>(lhs) < festd::bit_cast<uint32_t>(rhs);
        }

        friend bool operator>(const TextureSubresource lhs, const TextureSubresource rhs)
        {
            return festd::bit_cast<uint32_t>(lhs) > festd::bit_cast<uint32_t>(rhs);
        }

        friend bool operator==(const TextureSubresource lhs, const TextureSubresource rhs)
        {
            return festd::bit_cast<uint32_t>(lhs) == festd::bit_cast<uint32_t>(rhs);
        }

        friend bool operator!=(const TextureSubresource lhs, const TextureSubresource rhs)
        {
            return festd::bit_cast<uint32_t>(lhs) != festd::bit_cast<uint32_t>(rhs);
        }
    };

    inline const TextureSubresource TextureSubresource::kInvalid = festd::bit_cast<TextureSubresource>(kInvalidIndex);


    struct TextureSubresourceIterator final
    {
        struct Slice final
        {
            int32_t m_mipIndex : 16;
            int32_t m_arrayIndex : 16;
        };

        struct Iter final
        {
            TextureSubresource m_subresource;
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

        TextureSubresource m_subresource;

        explicit TextureSubresourceIterator(const TextureSubresource subresource)
            : m_subresource(subresource)
        {
        }

        explicit TextureSubresourceIterator(const TextureDesc imageDesc)
            : m_subresource(TextureSubresource::CreateWhole(imageDesc))
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


template<>
struct eastl::hash<FE::Graphics::Core::TextureSubresource>
{
    size_t operator()(const FE::Graphics::Core::TextureSubresource& subresource) const
    {
        return FE::festd::bit_cast<uint32_t>(subresource);
    }
};
