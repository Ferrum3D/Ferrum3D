#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Image/VKImage.h>
#include <OsGPU/Image/VKImageFormat.h>
#include <OsGPU/Image/VKImageSubresource.h>
#include <OsGPU/ImageView/VKImageView.h>

namespace FE::Osmium
{
    inline VkImageViewType VKConvert(ImageDim dim, bool isArray)
    {
        switch (dim)
        {
        case ImageDim::Image1D:
            return isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        case ImageDim::Image2D:
            return isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        case ImageDim::Image3D:
            FE_ASSERT_MSG(!isArray, "Array of 3D images is not allowed");
            return VK_IMAGE_VIEW_TYPE_3D;
        case ImageDim::ImageCubemap:
            return isArray ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        default:
            FE_UNREACHABLE("Invalid ImageDim");
            return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }

    const ImageViewDesc& VKImageView::GetDesc() const
    {
        return m_Desc;
    }

    VKImageView::VKImageView(VKDevice& dev, const ImageViewDesc& desc)
        : m_Desc(desc)
        , m_Device(&dev)
    {
        VkImageViewCreateInfo viewCI{};
        viewCI.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.components       = VkComponentMapping{};
        viewCI.format           = VKConvert(desc.Format);
        viewCI.image            = fe_assert_cast<VKImage*>(desc.Image)->Image;
        viewCI.viewType         = VKConvert(desc.Dimension, desc.SubresourceRange.ArraySliceCount != 1);
        viewCI.subresourceRange = VKConvert(desc.SubresourceRange);
        vkCreateImageView(m_Device->GetNativeDevice(), &viewCI, VK_NULL_HANDLE, &m_NativeView);
    }

    VKImageView::~VKImageView()
    {
        vkDestroyImageView(m_Device->GetNativeDevice(), m_NativeView, VK_NULL_HANDLE);
    }
} // namespace FE::Osmium
