#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Framebuffer/VKFramebuffer.h>
#include <OsGPU/ImageView/VKImageView.h>
#include <OsGPU/RenderPass/VKRenderPass.h>

namespace FE::Osmium
{
    VKFramebuffer::VKFramebuffer(VKDevice& dev, const FramebufferDesc& desc)
        : m_Device(&dev)
        , m_Desc(desc)
    {
        List<VkImageView> nativeRTVs;
        for (auto& view : desc.RenderTargetViews)
        {
            auto* vkView = fe_assert_cast<VKImageView*>(view);
            nativeRTVs.Push(vkView->GetNativeView());
        }

        VkFramebufferCreateInfo framebufferCI{};
        framebufferCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCI.renderPass      = fe_assert_cast<VKRenderPass*>(desc.RenderPass.Get())->GetNativeRenderPass();
        framebufferCI.attachmentCount = static_cast<UInt32>(nativeRTVs.Size());
        framebufferCI.pAttachments    = nativeRTVs.Data();
        framebufferCI.width           = desc.Width;
        framebufferCI.height          = desc.Height;
        framebufferCI.layers          = 1;

        vkCreateFramebuffer(m_Device->GetNativeDevice(), &framebufferCI, VK_NULL_HANDLE, &m_NativeFramebuffer);
    }

    FE_VK_OBJECT_DELETER(Framebuffer);

    VKFramebuffer::~VKFramebuffer()
    {
        FE_DELETE_VK_OBJECT(Framebuffer, m_NativeFramebuffer);
    }
} // namespace FE::Osmium
