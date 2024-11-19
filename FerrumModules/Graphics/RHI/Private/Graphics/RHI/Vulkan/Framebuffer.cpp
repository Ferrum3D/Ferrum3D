#include <FeCore/Containers/SmallVector.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/Framebuffer.h>
#include <Graphics/RHI/Vulkan/ImageView.h>
#include <Graphics/RHI/Vulkan/RenderPass.h>

namespace FE::Graphics::Vulkan
{
    Framebuffer::Framebuffer(RHI::Device* device)
    {
        m_device = device;
    }


    RHI::ResultCode Framebuffer::Init(const RHI::FramebufferDesc& desc)
    {
        m_desc = desc;

        festd::small_vector<VkImageView> nativeRTVs;
        for (const RHI::ImageView* view : desc.m_renderTargetViews)
        {
            nativeRTVs.push_back(NativeCast(view));
        }

        VkFramebufferCreateInfo framebufferCI{};
        framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCI.renderPass = NativeCast(desc.m_renderPass.Get());
        framebufferCI.attachmentCount = static_cast<uint32_t>(nativeRTVs.size());
        framebufferCI.pAttachments = nativeRTVs.data();
        framebufferCI.width = desc.m_width;
        framebufferCI.height = desc.m_height;
        framebufferCI.layers = 1;

        vkCreateFramebuffer(NativeCast(m_device), &framebufferCI, VK_NULL_HANDLE, &m_nativeFramebuffer);
        return RHI::ResultCode::kSuccess;
    }


    Framebuffer::~Framebuffer()
    {
        vkDestroyFramebuffer(NativeCast(m_device), m_nativeFramebuffer, nullptr);
    }
} // namespace FE::Graphics::Vulkan
