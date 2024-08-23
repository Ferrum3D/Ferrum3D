#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/Image.h>
#include <HAL/Vulkan/ImageFormat.h>
#include <HAL/Vulkan/ImageSubresource.h>
#include <HAL/Vulkan/ImageView.h>

namespace FE::Graphics::Vulkan
{
    inline static VkImageViewType VKConvert(HAL::ImageDim dim, bool isArray)
    {
        switch (dim)
        {
        case HAL::ImageDim::Image1D:
            return isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        case HAL::ImageDim::Image2D:
            return isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        case HAL::ImageDim::Image3D:
            FE_ASSERT_MSG(!isArray, "Array of 3D images is not allowed");
            return VK_IMAGE_VIEW_TYPE_3D;
        case HAL::ImageDim::ImageCubemap:
            return isArray ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        default:
            FE_UNREACHABLE("Invalid ImageDim");
            return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }


    const HAL::ImageViewDesc& ImageView::GetDesc() const
    {
        return m_Desc;
    }


    ImageView::ImageView(HAL::Device* pDevice)
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode ImageView::Init(const HAL::ImageViewDesc& desc)
    {
        m_Desc = desc;

        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.components = VkComponentMapping{};
        viewCI.format = VKConvert(desc.Format);
        viewCI.image = NativeCast(desc.Image);
        viewCI.viewType = VKConvert(desc.Dimension, desc.SubresourceRange.ArraySliceCount != 1);
        viewCI.subresourceRange = VKConvert(desc.SubresourceRange);
        vkCreateImageView(NativeCast(m_pDevice), &viewCI, VK_NULL_HANDLE, &m_NativeView);
        return HAL::ResultCode::Success;
    }


    ImageView::~ImageView()
    {
        vkDestroyImageView(NativeCast(m_pDevice), m_NativeView, nullptr);
    }
} // namespace FE::Graphics::Vulkan
