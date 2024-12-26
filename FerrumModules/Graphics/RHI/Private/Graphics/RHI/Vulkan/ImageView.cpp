#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/Image.h>
#include <Graphics/RHI/Vulkan/ImageFormat.h>
#include <Graphics/RHI/Vulkan/ImageSubresource.h>
#include <Graphics/RHI/Vulkan/ImageView.h>

namespace FE::Graphics::Vulkan
{
    inline static VkImageViewType VKConvert(RHI::ImageDimension dim, bool isArray)
    {
        switch (dim)
        {
        case RHI::ImageDimension::k1D:
            return isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        case RHI::ImageDimension::k2D:
            return isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        case RHI::ImageDimension::k3D:
            FE_AssertMsg(!isArray, "Array of 3D images is not allowed");
            return VK_IMAGE_VIEW_TYPE_3D;
        case RHI::ImageDimension::kCubemap:
            return isArray ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        default:
            FE_AssertMsg(false, "Invalid ImageDim");
            return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }


    const RHI::ImageViewDesc& ImageView::GetDesc() const
    {
        return m_desc;
    }


    ImageView::ImageView(RHI::Device* device)
    {
        m_device = device;
    }


    RHI::ResultCode ImageView::Init(const RHI::ImageViewDesc& desc)
    {
        m_desc = desc;

        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.components = VkComponentMapping{};
        viewCI.format = VKConvert(desc.m_format);
        viewCI.image = NativeCast(desc.m_image);
        viewCI.viewType = VKConvert(desc.m_dimension, desc.m_subresourceRange.m_arraySliceCount != 1);
        viewCI.subresourceRange = VKConvert(desc.m_subresourceRange);
        vkCreateImageView(NativeCast(m_device), &viewCI, VK_NULL_HANDLE, &m_nativeView);
        return RHI::ResultCode::kSuccess;
    }


    ImageView::~ImageView()
    {
        vkDestroyImageView(NativeCast(m_device), m_nativeView, nullptr);
    }
} // namespace FE::Graphics::Vulkan
