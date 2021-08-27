#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Framebuffer/VKFramebuffer.h>
#include <FeGPU/ImageView/VKImageView.h>
#include <FeGPU/RenderPass/VKRenderPass.h>

namespace FE::GPU
{
    VKFramebuffer::VKFramebuffer(VKDevice& dev, const FramebufferDesc& desc)
        : m_Device(&dev)
        , m_Desc(desc)
    {
        Vector<vk::ImageView> nativeRTVs;
        m_RTVs = desc.RenderTargetViews;
        for (auto& view : desc.RenderTargetViews)
        {
            auto* vkView = fe_assert_cast<VKImageView*>(view.GetRaw());
            nativeRTVs.push_back(vkView->GetNativeView());
        }

        vk::FramebufferCreateInfo framebufferCI{};
        framebufferCI.renderPass      = fe_assert_cast<VKRenderPass*>(desc.RenderPass.GetRaw())->GetNativeRenderPass();
        framebufferCI.attachmentCount = static_cast<UInt32>(nativeRTVs.size());
        framebufferCI.pAttachments    = nativeRTVs.data();
        framebufferCI.width           = desc.Width;
        framebufferCI.height          = desc.Height;
        framebufferCI.layers          = 1;

        m_NativeFramebuffer = m_Device->GetNativeDevice().createFramebufferUnique(framebufferCI);
    }
} // namespace FE::GPU
