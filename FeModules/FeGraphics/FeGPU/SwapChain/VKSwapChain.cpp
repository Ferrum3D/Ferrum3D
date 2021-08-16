#include <FeGPU/Adapter/VKAdapter.h>
#include <FeGPU/CommandQueue/VKCommandQueue.h>
#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Fence/VKFence.h>
#include <FeGPU/Image/VKImage.h>
#include <FeGPU/ImageView/VKImageView.h>
#include <FeGPU/SwapChain/VKSwapChain.h>

namespace FE::GPU
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
            m_Images.push_back(static_pointer_cast<IImage>(backBuffer));

            ImageViewDesc viewDesc{};
            viewDesc.SubresourceRange             = {};
            viewDesc.SubresourceRange.AspectFlags = ImageAspectFlags::Color;
            viewDesc.Image                        = static_pointer_cast<IImage>(backBuffer);
            viewDesc.Format                       = m_Desc.Format;
            viewDesc.Dimension                    = ImageDim::Image2D;
            m_ImageViews.push_back(MakeShared<VKImageView>(*m_Device, viewDesc));
        }
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
        vk::PresentModeKHR mode = vk::PresentModeKHR::eFifo;

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
        swapChainCI.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        swapChainCI.imageSharingMode = vk::SharingMode::eExclusive;
        swapChainCI.preTransform     = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        swapChainCI.presentMode      = mode;
        swapChainCI.clipped          = true;

        m_NativeSwapChain = m_Device->GetNativeDevice().createSwapchainKHRUnique(swapChainCI);
    }

    VKSwapChain::~VKSwapChain()
    {
        m_Device->GetNativeDevice().waitIdle();
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

    void VKSwapChain::Present()
    {
        vk::PresentInfoKHR presentInfo{};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &m_RenderFinishedSemaphore.get();
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &m_NativeSwapChain.get();
        presentInfo.pImageIndices      = &m_FrameIndex;
        FE_VK_ASSERT(m_Queue->GetNativeQueue().presentKHR(presentInfo));
        m_Queue->GetNativeQueue().waitIdle(); // TODO: frames in-flight

        AcquireNextImage(&m_FrameIndex);
    }

    void VKSwapChain::AcquireNextImage(UInt32* index)
    {
        FE_VK_ASSERT(m_Device->GetNativeDevice().acquireNextImageKHR(
            m_NativeSwapChain.get(), static_cast<UInt64>(-1), m_ImageAvailableSemaphore.get(), nullptr, index));
    }
} // namespace FE::GPU
