#pragma once
#include <Graphics/RHI/ImageEnums.h>

namespace FE::Graphics::RHI
{
    struct ImageSubresource final
    {
        ImageAspect m_aspect : 8;
        uint32_t m_mipSlice : 8;
        uint32_t m_arraySlice : 16;

        ImageSubresource()
            : m_aspect(ImageAspect::kColor)
            , m_mipSlice(0)
            , m_arraySlice(0)
        {
        }
    };


    struct ImageSubresourceRange final
    {
        uint32_t m_minMipSlice : 8;
        uint32_t m_mipSliceCount : 8;
        uint32_t m_minArraySlice : 16;
        uint32_t m_arraySliceCount : 16;

        ImageAspectFlags m_aspectFlags : 16;

        ImageSubresourceRange()
            : m_minMipSlice(0)
            , m_mipSliceCount(1)
            , m_minArraySlice(0)
            , m_arraySliceCount(1)
            , m_aspectFlags(ImageAspectFlags::kAll)
        {
        }

        ImageSubresourceRange(ImageSubresource subresource)
        {
            m_aspectFlags = static_cast<ImageAspectFlags>(1 << enum_cast(subresource.m_aspect));
            m_minMipSlice = subresource.m_mipSlice;
            m_minArraySlice = subresource.m_arraySlice;
            m_mipSliceCount = 1;
            m_arraySliceCount = 1;
        }
    };
} // namespace FE::Graphics::RHI
