#include <OsGPU/Adapter/VKAdapter.h>
#include <OsGPU/CommandQueue/VKCommandQueue.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Image/VKImage.h>
#include <OsGPU/ImageView/IImageView.h>
#include <OsGPU/Instance/VKInstance.h>
#include <OsGPU/SwapChain/VKSwapChain.h>

namespace FE::Osmium
{
    bool VKSwapChain::ValidateDimensions(const SwapChainDesc& swapChainDesc) const
    {
        return m_Capabilities.minImageExtent.width <= swapChainDesc.ImageWidth
            && m_Capabilities.minImageExtent.height <= swapChainDesc.ImageHeight
            && m_Capabilities.maxImageExtent.width >= swapChainDesc.ImageWidth
            && m_Capabilities.maxImageExtent.height >= swapChainDesc.ImageHeight;
    }

    VKSwapChain::VKSwapChain(VKDevice& dev, const SwapChainDesc& desc)
        : m_Device(&dev)
        , m_Desc(desc)
        , m_Queue(fe_assert_cast<VKCommandQueue*>(desc.Queue))
    {
        auto& instance = *fe_assert_cast<VKInstance*>(&m_Device->GetInstance());

        BuildNativeSwapChain(instance);

        auto images = m_Device->GetNativeDevice().getSwapchainImagesKHR<StdHeapAllocator<vk::Image>>(m_NativeSwapChain.get());

        m_Desc.Format = VKConvert(m_ColorFormat.format);
        for (auto& image : images)
        {
            auto width        = m_Desc.ImageWidth;
            auto height       = m_Desc.ImageHeight;
            auto backBuffer   = MakeShared<VKImage>(*m_Device);
            backBuffer->Image = image;
            backBuffer->Desc  = ImageDesc::Img2D(ImageBindFlags::Color, width, height, m_Desc.Format);
            m_Images.Push(static_pointer_cast<IImage>(backBuffer));

            m_ImageViews.Push(backBuffer->CreateRenderTargetView());
        }

        for (size_t i = 0; i < m_Desc.FrameCount; ++i)
        {
            m_RenderFinishedSemaphores.push_back(m_Device->GetNativeDevice().createSemaphoreUnique(vk::SemaphoreCreateInfo{}));
            m_ImageAvailableSemaphores.push_back(m_Device->GetNativeDevice().createSemaphoreUnique(vk::SemaphoreCreateInfo{}));
        }

        AcquireNextImage(&m_ImageIndex);
        *m_CurrentImageAvailableSemaphore = m_ImageAvailableSemaphores[m_FrameIndex].get();
        *m_CurrentRenderFinishedSemaphore = m_RenderFinishedSemaphores[m_FrameIndex].get();
    }

    void VKSwapChain::BuildNativeSwapChain(VKInstance& instance)
    {
#if FE_WINDOWS
        vk::Win32SurfaceCreateInfoKHR surfaceCI{};
        surfaceCI.hinstance = GetModuleHandle(nullptr);
        surfaceCI.hwnd      = static_cast<HWND>(m_Desc.NativeWindowHandle);
        m_Surface           = instance.GetNativeInstance().createWin32SurfaceKHRUnique(surfaceCI);
#else
#    error platform not supported
#endif

        auto& pd     = fe_assert_cast<VKAdapter*>(&m_Device->GetAdapter())->GetNativeAdapter();
        auto formats = pd.getSurfaceFormatsKHR<StdHeapAllocator<vk::SurfaceFormatKHR>>(m_Surface.get());
        FE_ASSERT(pd.getSurfaceSupportKHR(m_Queue->GetDesc().QueueFamilyIndex, m_Surface.get()));

        constexpr auto preferredFormat = Format::B8G8R8A8_SRGB;
        m_ColorFormat.format           = vk::Format::eUndefined;
        for (auto& fmt : formats)
        {
            if (fmt.format == preferredFormat && fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                m_ColorFormat = fmt;
            }
        }
        if (m_ColorFormat == vk::Format::eUndefined)
        {
            FE_LOG_WARNING("SwapChain format {} is not supported, using the first supported one", preferredFormat);
            m_ColorFormat = formats.front();
        }

        m_Capabilities = pd.getSurfaceCapabilitiesKHR(m_Surface.get());
        if (!ValidateDimensions(m_Desc))
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
        auto mode = vk::PresentModeKHR::eFifo;

        // If v-sync is disabled, try to use either immediate or mailbox (if supported).
        if (!m_Desc.VerticalSync)
        {
            auto supportedModes = pd.getSurfacePresentModesKHR<StdHeapAllocator<vk::PresentModeKHR>>(m_Surface.get());
            auto preferredModes = { vk::PresentModeKHR::eImmediate, vk::PresentModeKHR::eMailbox };

            for (auto& supported : supportedModes)
            {
                for (auto& preferred : preferredModes)
                {
                    if (supported == preferred)
                    {
                        mode = supported;
                    }
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
        swapChainCI.compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapChainCI.imageSharingMode      = vk::SharingMode::eExclusive;
        swapChainCI.preTransform          = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        swapChainCI.presentMode           = mode;
        swapChainCI.clipped               = true;
        swapChainCI.pQueueFamilyIndices   = &m_Queue->GetDesc().QueueFamilyIndex;
        swapChainCI.queueFamilyIndexCount = 1;

        m_NativeSwapChain = m_Device->GetNativeDevice().createSwapchainKHRUnique(swapChainCI);

        m_CurrentImageAvailableSemaphore = &m_Device->AddWaitSemaphore();
        m_CurrentRenderFinishedSemaphore = &m_Device->AddSignalSemaphore();
    }

    VKSwapChain::~VKSwapChain() = default;

    const SwapChainDesc& VKSwapChain::GetDesc()
    {
        return m_Desc;
    }

    UInt32 VKSwapChain::GetCurrentImageIndex()
    {
        return m_ImageIndex;
    }

    UInt32 VKSwapChain::GetImageCount()
    {
        return static_cast<UInt32>(m_Images.Size());
    }

    IImage* VKSwapChain::GetImage(UInt32 index)
    {
        return m_Images[index].GetRaw();
    }

    IImage* VKSwapChain::GetCurrentImage()
    {
        return m_Images[m_ImageIndex].GetRaw();
    }

    void VKSwapChain::Present()
    {
        vk::PresentInfoKHR presentInfo{};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &m_RenderFinishedSemaphores[m_FrameIndex].get();
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &m_NativeSwapChain.get();
        presentInfo.pImageIndices      = &m_ImageIndex;
        FE_VK_ASSERT(m_Queue->GetNativeQueue().presentKHR(presentInfo));

        m_FrameIndex                      = (m_FrameIndex + 1) % m_Desc.FrameCount;
        *m_CurrentImageAvailableSemaphore = m_ImageAvailableSemaphores[m_FrameIndex].get();
        *m_CurrentRenderFinishedSemaphore = m_RenderFinishedSemaphores[m_FrameIndex].get();
        AcquireNextImage(&m_ImageIndex);
    }

    void VKSwapChain::AcquireNextImage(UInt32* index)
    {
        vk::Semaphore semaphore = m_ImageAvailableSemaphores[m_FrameIndex].get();
        FE_VK_ASSERT(m_Device->GetNativeDevice().acquireNextImageKHR(
            m_NativeSwapChain.get(), static_cast<UInt64>(-1), semaphore, nullptr, index));
    }

    List<Shared<IImageView>> VKSwapChain::GetRTVs()
    {
        return m_ImageViews;
    }

    UInt32 VKSwapChain::GetCurrentFrameIndex()
    {
        return m_FrameIndex;
    }
} // namespace FE::Osmium
