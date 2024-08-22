#include <FeCore/Containers/SmallVector.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/Framebuffer.h>
#include <HAL/Vulkan/ImageView.h>
#include <HAL/Vulkan/RenderPass.h>

namespace FE::Graphics::Vulkan
{
    Framebuffer::Framebuffer(HAL::Device* pDevice)
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode Framebuffer::Init(const HAL::FramebufferDesc& desc)
    {
        m_Desc = desc;

        festd::small_vector<VkImageView> nativeRTVs;
        for (const HAL::ImageView* view : desc.RenderTargetViews)
        {
            nativeRTVs.push_back(ImplCast(view)->GetNativeView());
        }

        VkFramebufferCreateInfo framebufferCI{};
        framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCI.renderPass = ImplCast(desc.RenderPass.Get())->GetNativeRenderPass();
        framebufferCI.attachmentCount = static_cast<uint32_t>(nativeRTVs.size());
        framebufferCI.pAttachments = nativeRTVs.data();
        framebufferCI.width = desc.Width;
        framebufferCI.height = desc.Height;
        framebufferCI.layers = 1;

        vkCreateFramebuffer(ImplCast(m_pDevice)->GetNativeDevice(), &framebufferCI, VK_NULL_HANDLE, &m_NativeFramebuffer);
        return HAL::ResultCode::Success;
    }


    Framebuffer::~Framebuffer()
    {
        vkDestroyFramebuffer(ImplCast(m_pDevice)->GetNativeDevice(), m_NativeFramebuffer, nullptr);
    }
} // namespace FE::Graphics::Vulkan
