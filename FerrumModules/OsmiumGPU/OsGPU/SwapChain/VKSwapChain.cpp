#include <OsGPU/Adapter/VKAdapter.h>
#include <OsGPU/CommandQueue/VKCommandQueue.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Image/VKImage.h>
#include <OsGPU/ImageView/IImageView.h>
#include <OsGPU/Instance/VKInstance.h>
#include <OsGPU/SwapChain/VKSwapChain.h>
#include <algorithm>

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
        ZoneScoped;
        auto& instance = *fe_assert_cast<VKInstance*>(&m_Device->GetInstance());

        BuildNativeSwapChain(instance);

        vkGetSwapchainImagesKHR(m_Device->GetNativeDevice(), m_NativeSwapChain, &m_Desc.ImageCount, nullptr);
        eastl::vector<VkImage> images(m_Desc.ImageCount, VK_NULL_HANDLE);
        vkGetSwapchainImagesKHR(m_Device->GetNativeDevice(), m_NativeSwapChain, &m_Desc.ImageCount, images.data());

        auto width = m_Desc.ImageWidth;
        auto height = m_Desc.ImageHeight;
        m_Desc.Format = VKConvert(m_ColorFormat.format);
        for (auto& image : images)
        {
            VKImage* backBuffer = Rc<VKImage>::DefaultNew(*m_Device);
            backBuffer->Image = image;
            backBuffer->Desc = ImageDesc::Img2D(ImageBindFlags::Color, width, height, m_Desc.Format);
            m_Images.push_back(backBuffer);

            m_ImageViews.push_back(backBuffer->CreateView(ImageAspectFlags::Color));
        }

        auto depthImageDesc = ImageDesc::Img2D(ImageBindFlags::Depth, width, height, Format::D32_SFloat);
        m_DepthImage = dev.CreateImage(depthImageDesc);
        m_DepthImage->AllocateMemory(MemoryType::DeviceLocal);
        m_DepthImageView = m_DepthImage->CreateView(ImageAspectFlags::Depth);

        for (size_t i = 0; i < m_Desc.FrameCount; ++i)
        {
            VkSemaphoreCreateInfo semaphoreCI{};
            semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            vkCreateSemaphore(m_Device->GetNativeDevice(), &semaphoreCI, VK_NULL_HANDLE, &m_RenderFinishedSemaphores.push_back());
            vkCreateSemaphore(m_Device->GetNativeDevice(), &semaphoreCI, VK_NULL_HANDLE, &m_ImageAvailableSemaphores.push_back());
        }

        AcquireNextImage(&m_ImageIndex);
        *m_CurrentImageAvailableSemaphore = m_ImageAvailableSemaphores[m_FrameIndex];
        *m_CurrentRenderFinishedSemaphore = m_RenderFinishedSemaphores[m_FrameIndex];
    }

    void VKSwapChain::BuildNativeSwapChain(VKInstance& instance)
    {
#if FE_WINDOWS
        VkWin32SurfaceCreateInfoKHR surfaceCI{};
        surfaceCI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCI.hinstance = GetModuleHandle(nullptr);
        surfaceCI.hwnd = static_cast<HWND>(m_Desc.NativeWindowHandle);
        vkCreateWin32SurfaceKHR(instance.GetNativeInstance(), &surfaceCI, VK_NULL_HANDLE, &m_Surface);
#else
#    error platform not supported
#endif

        auto pd = fe_assert_cast<VKAdapter*>(&m_Device->GetAdapter())->GetNativeAdapter();

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(pd, m_Surface, &formatCount, nullptr);
        eastl::vector formats(formatCount, VkSurfaceFormatKHR{});
        vkGetPhysicalDeviceSurfaceFormatsKHR(pd, m_Surface, &formatCount, formats.data());
        VkBool32 formatSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(pd, m_Queue->GetDesc().QueueFamilyIndex, m_Surface, &formatSupported);
        FE_ASSERT(formatSupported);

        constexpr auto preferredFormat = Format::B8G8R8A8_SRGB;
        m_ColorFormat.format = VK_FORMAT_UNDEFINED;
        for (auto& fmt : formats)
        {
            if (fmt.format == preferredFormat && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_ColorFormat = fmt;
            }
        }
        if (m_ColorFormat.format == VK_FORMAT_UNDEFINED)
        {
            FE_LOG_WARNING("SwapChain format {} is not supported, using the first supported one", preferredFormat);
            m_ColorFormat = formats.front();
        }

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, m_Surface, &m_Capabilities);
        if (!ValidateDimensions(m_Desc))
        {
            auto min = m_Capabilities.minImageExtent;
            auto max = m_Capabilities.maxImageExtent;
            auto width = std::clamp(m_Desc.ImageWidth, min.width, max.width);
            auto height = std::clamp(m_Desc.ImageHeight, min.height, max.height);
            FE_LOG_WARNING("Requested swap chain size ({}, {}) was resized to ({}, {}) according to capabilities",
                           m_Desc.ImageWidth,
                           m_Desc.ImageHeight,
                           width,
                           height);
            m_Desc.ImageWidth = width;
            m_Desc.ImageHeight = height;
        }
        auto mode = VK_PRESENT_MODE_FIFO_KHR;

        // If v-sync is disabled, try to use either immediate or mailbox (if supported).
        if (!m_Desc.VerticalSync)
        {
            uint32_t supportedModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(pd, m_Surface, &supportedModeCount, nullptr);
            eastl::vector<VkPresentModeKHR> supportedModes(supportedModeCount, VkPresentModeKHR{});
            vkGetPhysicalDeviceSurfacePresentModesKHR(pd, m_Surface, &supportedModeCount, supportedModes.data());
            auto preferredModes = { VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR };

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

        if (mode == VK_PRESENT_MODE_FIFO_KHR && !m_Desc.VerticalSync)
        {
            FE_LOG_WARNING("V-Sync is force enabled, because FIFO is the only supported present mode");
        }

        VkSwapchainCreateInfoKHR swapChainCI{};
        swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainCI.surface = m_Surface;
        swapChainCI.imageArrayLayers = 1;
        swapChainCI.minImageCount = m_Desc.ImageCount;
        swapChainCI.imageExtent = m_Capabilities.currentExtent;
        swapChainCI.imageFormat = m_ColorFormat.format;
        swapChainCI.imageColorSpace = m_ColorFormat.colorSpace;
        swapChainCI.imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapChainCI.presentMode = mode;
        swapChainCI.clipped = true;
        swapChainCI.pQueueFamilyIndices = &m_Queue->GetDesc().QueueFamilyIndex;
        swapChainCI.queueFamilyIndexCount = 1;

        vkCreateSwapchainKHR(m_Device->GetNativeDevice(), &swapChainCI, VK_NULL_HANDLE, &m_NativeSwapChain);

        m_CurrentImageAvailableSemaphore = &m_Device->AddWaitSemaphore();
        m_CurrentRenderFinishedSemaphore = &m_Device->AddSignalSemaphore();
    }

    VKSwapChain::~VKSwapChain()
    {
        for (auto& semaphore : m_ImageAvailableSemaphores)
        {
            vkDestroySemaphore(m_Device->GetNativeDevice(), semaphore, VK_NULL_HANDLE);
        }

        for (auto& semaphore : m_RenderFinishedSemaphores)
        {
            vkDestroySemaphore(m_Device->GetNativeDevice(), semaphore, VK_NULL_HANDLE);
        }

        vkDestroySwapchainKHR(m_Device->GetNativeDevice(), m_NativeSwapChain, VK_NULL_HANDLE);
        auto instance = fe_assert_cast<VKInstance*>(&m_Device->GetInstance())->GetNativeInstance();
        vkDestroySurfaceKHR(instance, m_Surface, VK_NULL_HANDLE);
    }

    const SwapChainDesc& VKSwapChain::GetDesc()
    {
        return m_Desc;
    }

    uint32_t VKSwapChain::GetCurrentImageIndex()
    {
        return m_ImageIndex;
    }

    uint32_t VKSwapChain::GetImageCount()
    {
        return static_cast<uint32_t>(m_Images.size());
    }

    IImage* VKSwapChain::GetImage(uint32_t index)
    {
        return m_Images[index].Get();
    }

    IImage* VKSwapChain::GetCurrentImage()
    {
        return m_Images[m_ImageIndex].Get();
    }

    void VKSwapChain::Present()
    {
        ZoneScoped;
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_FrameIndex];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_NativeSwapChain;
        presentInfo.pImageIndices = &m_ImageIndex;
        FE_VK_ASSERT(vkQueuePresentKHR(m_Queue->GetNativeQueue(), &presentInfo));

        m_FrameIndex = (m_FrameIndex + 1) % m_Desc.FrameCount;
        *m_CurrentImageAvailableSemaphore = m_ImageAvailableSemaphores[m_FrameIndex];
        *m_CurrentRenderFinishedSemaphore = m_RenderFinishedSemaphores[m_FrameIndex];
        AcquireNextImage(&m_ImageIndex);
    }

    void VKSwapChain::AcquireNextImage(uint32_t* index)
    {
        VkSemaphore semaphore = m_ImageAvailableSemaphores[m_FrameIndex];
        FE_VK_ASSERT(vkAcquireNextImageKHR(
            m_Device->GetNativeDevice(), m_NativeSwapChain, static_cast<uint64_t>(-1), semaphore, nullptr, index));
    }

    eastl::vector<IImageView*> VKSwapChain::GetRTVs()
    {
        eastl::vector<IImageView*> result;
        result.reserve(m_ImageViews.size());
        for (auto& view : m_ImageViews)
        {
            result.push_back(view.Get());
        }

        return result;
    }

    IImageView* VKSwapChain::GetDSV()
    {
        return m_DepthImageView.Get();
    }

    uint32_t VKSwapChain::GetCurrentFrameIndex()
    {
        return m_FrameIndex;
    }
} // namespace FE::Osmium
