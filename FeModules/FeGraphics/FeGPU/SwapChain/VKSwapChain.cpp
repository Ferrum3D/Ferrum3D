#include <FeGPU/Adapter/VKAdapter.h>
#include <FeGPU/CommandQueue/VKCommandQueue.h>
#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Fence/VKFence.h>
#include <FeGPU/Image/VKImage.h>
#include <FeGPU/Instance/VKInstance.h>
#include <FeGPU/SwapChain/VKSwapChain.h>

namespace FE::GPU
{
    bool VKSwapChain::ValidateDimentions(const SwapChainDesc& swapChainDesc)
    {
        return m_Capabilities.minImageExtent.width <= swapChainDesc.ImageWidth
            && m_Capabilities.minImageExtent.height <= swapChainDesc.ImageHeight
            && m_Capabilities.maxImageExtent.width >= swapChainDesc.ImageWidth
            && m_Capabilities.maxImageExtent.height >= swapChainDesc.ImageHeight;
    }

    VKSwapChain::VKSwapChain(VKDevice& dev, const SwapChainDesc& desc)
        : m_Device(&dev)
        , m_Desc(desc)
        , m_Queue(static_cast<VKCommandQueue*>(desc.Queue))
    {
        auto& instance = static_cast<VKInstance&>(m_Device->GetInstance());

#if FE_WINDOWS
        vk::Win32SurfaceCreateInfoKHR surfaceCI{};
        surfaceCI.hinstance = GetModuleHandle(nullptr);
        surfaceCI.hwnd      = static_cast<HWND>(m_Desc.NativeWindowHandle);
        m_Surface           = instance.GetNativeInstance().createWin32SurfaceKHRUnique(surfaceCI);
#else
#    error platform not supported
#endif

        auto& pd     = static_cast<VKAdapter&>(m_Device->GetAdapter()).GetNativeAdapter();
        auto formats = pd.getSurfaceFormatsKHR<StdHeapAllocator<vk::SurfaceFormatKHR>>(m_Surface.get());
        FE_ASSERT(pd.getSurfaceSupportKHR(m_Queue->GetDesc().QueueFamilyIndex, m_Surface.get()));

        constexpr auto preferredFormat = Format::B8G8R8A8_SRGB;
        m_ColorFormat.format           = vk::Format::eUndefined;
        for (auto& fmt : formats)
            if (fmt.format == preferredFormat && fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                m_ColorFormat = fmt;
        if (m_ColorFormat == vk::Format::eUndefined)
        {
            FE_LOG_WARNING("SwapChain format {} is not supported, using the first supported one", preferredFormat);
            m_ColorFormat = formats.front();
        }

        m_Capabilities = pd.getSurfaceCapabilitiesKHR(m_Surface.get());
        if (!ValidateDimentions(m_Desc))
        {
            auto min    = m_Capabilities.minImageExtent;
            auto max    = m_Capabilities.maxImageExtent;
            auto width  = std::clamp(m_Desc.ImageWidth, min.width, max.width);
            auto height = std::clamp(m_Desc.ImageHeight, min.height, max.height);
            FE_LOG_WARNING(
                "Requested swap chain size ({}, {}) was resized to ({}, {}) according to capabilities", m_Desc.ImageWidth,
                m_Desc.ImageHeight, width, height);
            m_Desc.ImageWidth  = width;
            m_Desc.ImageHeight = height;
        }
        vk::PresentModeKHR mode = vk::PresentModeKHR::eFifo;

        // is v-sync is disabled, try to use either immediate or mailbox (if supported)
        if (!m_Desc.VerticalSync)
        {
            auto supportedModes = pd.getSurfacePresentModesKHR<StdHeapAllocator<vk::PresentModeKHR>>(m_Surface.get());
            auto preferredModes = { vk::PresentModeKHR::eImmediate, vk::PresentModeKHR::eMailbox };

            for (auto& supported : supportedModes)
            {
                for (auto& preferred : preferredModes)
                {
                    if (supported == preferred)
                        mode = supported;
                }
            }
        }

        if (mode == vk::PresentModeKHR::eFifo && !m_Desc.VerticalSync)
        {
            FE_LOG_WARNING("V-Sync is force enabled, because FIFO is the only supported present mode");
        }

        vk::SwapchainCreateInfoKHR swapChainCI{};
        swapChainCI.surface          = m_Surface.get();
        swapChainCI.imageArrayLayers = 1;
        swapChainCI.minImageCount    = m_Desc.ImageCount;
        swapChainCI.imageExtent      = m_Capabilities.currentExtent;
        swapChainCI.imageFormat      = m_ColorFormat.format;
        swapChainCI.imageColorSpace  = m_ColorFormat.colorSpace;
        swapChainCI.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst
            | vk::ImageUsageFlagBits::eInputAttachment;
        swapChainCI.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapChainCI.imageSharingMode = vk::SharingMode::eExclusive;
        swapChainCI.preTransform     = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        swapChainCI.presentMode      = mode;
        swapChainCI.clipped          = true;

        m_NativeSwapChain = m_Device->GetNativeDevice().createSwapchainKHRUnique(swapChainCI);
        auto images = m_Device->GetNativeDevice().getSwapchainImagesKHR<StdHeapAllocator<vk::Image>>(m_NativeSwapChain.get());

        m_CmdBuffer = m_Device->CreateCommandBuffer(CommandQueueClass::Graphics);
        m_CmdBuffer->Begin();
        for (auto& image : images)
        {
            auto width        = m_Desc.ImageWidth;
            auto height       = m_Desc.ImageHeight;
            auto backbuffer   = MakeShared<VKImage>(*m_Device);
            backbuffer->Image = image;
            backbuffer->Desc  = ImageDesc::Img2D(ImageBindFlags::Color, width, height, VKConvert(m_ColorFormat.format));
            ResourceTransitionBarrierDesc barrierDesc{};
            barrierDesc.Image                   = backbuffer.GetRaw();
            barrierDesc.StateBefore             = ResourceState::None;
            barrierDesc.StateAfter              = ResourceState::Present;
            barrierDesc.SubresourceRange.Apsect = ImageAspect::Color;
            m_CmdBuffer->ResourceTransitionBarriers({ barrierDesc });
            m_Images.push_back(StaticPtrCast<IImage>(backbuffer));
        }
        m_CmdBuffer->End();

        m_ImageAvailableSemaphore = m_Device->GetNativeDevice().createSemaphoreUnique(vk::SemaphoreCreateInfo{});
        m_RenderFinishedSemaphore = m_Device->GetNativeDevice().createSemaphoreUnique(vk::SemaphoreCreateInfo{});
        m_Fence                   = m_Device->CreateFence(0);
        m_Queue->SubmitBuffers({ m_CmdBuffer });
        m_Queue->SignalFence(m_Fence, 1);
    }

    VKSwapChain::~VKSwapChain()
    {
        m_Fence->Wait(1);
    }

    const SwapChainDesc& VKSwapChain::GetDesc()
    {
        return m_Desc;
    }

    UInt32 VKSwapChain::GetCurrentImageIndex()
    {
        return m_FrameIndex;
    }

    UInt32 VKSwapChain::GetImageCount()
    {
        return static_cast<UInt32>(m_Images.size());
    }

    IImage* VKSwapChain::GetImage(UInt32 index)
    {
        return m_Images[index].GetRaw();
    }

    IImage* VKSwapChain::GetCurrentImage()
    {
        return m_Images[m_FrameIndex].GetRaw();
    }

    UInt32 VKSwapChain::NextImage([[maybe_unused]] const RefCountPtr<IFence>& fence, UInt64 signal)
    {
        FE_VK_ASSERT(m_Device->GetNativeDevice().acquireNextImageKHR(
            m_NativeSwapChain.get(), UInt64(-1), m_ImageAvailableSemaphore.get(), nullptr, &m_FrameIndex));

        auto* vkfence    = static_cast<VKFence*>(m_Fence.GetRaw());
        UInt64 waitValue = UInt64(-1);

        vk::TimelineSemaphoreSubmitInfo timelineInfo{};
        timelineInfo.waitSemaphoreValueCount   = 1;
        timelineInfo.pWaitSemaphoreValues      = &waitValue;
        timelineInfo.signalSemaphoreValueCount = 1;
        timelineInfo.pSignalSemaphoreValues    = &signal;

        vk::SubmitInfo submitInfo{};
        submitInfo.pNext              = &timelineInfo;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores    = &m_ImageAvailableSemaphore.get();

        vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eTransfer;
        submitInfo.pWaitDstStageMask            = &waitDstStageMask;
        submitInfo.signalSemaphoreCount         = 1;
        submitInfo.pSignalSemaphores            = &vkfence->GetNativeSemaphore();
        FE_VK_ASSERT(m_Queue->GetNativeQueue().submit(1, &submitInfo, {}));

        return m_FrameIndex;
    }

    void VKSwapChain::Present([[maybe_unused]] const RefCountPtr<IFence>& fence, UInt64 wait)
    {
        auto* vkfence      = static_cast<VKFence*>(m_Fence.GetRaw());
        UInt64 signalValue = UInt64(-1);

        vk::TimelineSemaphoreSubmitInfo timelineInfo{};
        timelineInfo.waitSemaphoreValueCount   = 1;
        timelineInfo.pWaitSemaphoreValues      = &wait;
        timelineInfo.signalSemaphoreValueCount = 1;
        timelineInfo.pSignalSemaphoreValues    = &signalValue;

        vk::SubmitInfo submitInfo{};
        submitInfo.pNext              = &timelineInfo;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores    = &vkfence->GetNativeSemaphore();

        vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eTransfer;
        submitInfo.pWaitDstStageMask            = &waitDstStageMask;
        submitInfo.signalSemaphoreCount         = 1;
        submitInfo.pSignalSemaphores            = &m_RenderFinishedSemaphore.get();
        FE_VK_ASSERT(m_Queue->GetNativeQueue().submit(1, &submitInfo, {}));

        vk::PresentInfoKHR presentInfo{};
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &m_NativeSwapChain.get();
        presentInfo.pImageIndices      = &m_FrameIndex;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &m_RenderFinishedSemaphore.get();
        FE_VK_ASSERT(m_Queue->GetNativeQueue().presentKHR(presentInfo));
    }
} // namespace FE::GPU
