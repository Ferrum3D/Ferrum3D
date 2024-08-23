#include <FeCore/Containers/SmallVector.h>
#include <HAL/ImageView.h>
#include <HAL/Vulkan/CommandQueue.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/DeviceFactory.h>
#include <HAL/Vulkan/Image.h>
#include <HAL/Vulkan/ImageView.h>
#include <HAL/Vulkan/Swapchain.h>
#include <algorithm>

namespace FE::Graphics::Vulkan
{
    bool Swapchain::ValidateDimensions(const HAL::SwapchainDesc& swapchainDesc) const
    {
        return m_Capabilities.minImageExtent.width <= swapchainDesc.ImageWidth
            && m_Capabilities.minImageExtent.height <= swapchainDesc.ImageHeight
            && m_Capabilities.maxImageExtent.width >= swapchainDesc.ImageWidth
            && m_Capabilities.maxImageExtent.height >= swapchainDesc.ImageHeight;
    }


    Swapchain::Swapchain(HAL::Device* pDevice, HAL::Image* pDepthImage)
        : m_DepthImage(ImplCast(pDepthImage))
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode Swapchain::Init(const HAL::SwapchainDesc& desc)
    {
        ZoneScoped;
        m_Desc = desc;
        m_Queue = ImplCast(desc.Queue);

        BuildNativeSwapchain();

        const VkDevice device = NativeCast(m_pDevice);
        vkGetSwapchainImagesKHR(device, m_NativeSwapchain, &m_Desc.ImageCount, nullptr);
        festd::small_vector<VkImage> images(m_Desc.ImageCount, VK_NULL_HANDLE);
        vkGetSwapchainImagesKHR(device, m_NativeSwapchain, &m_Desc.ImageCount, images.data());

        DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();

        const uint32_t width = m_Desc.ImageWidth;
        const uint32_t height = m_Desc.ImageHeight;
        m_Desc.Format = VKConvert(m_ColorFormat.format);
        for (auto& image : images)
        {
            Rc backBuffer = ImplCast(pServiceProvider->ResolveRequired<HAL::Image>());
            const auto imageDesc = HAL::ImageDesc::Img2D(HAL::ImageBindFlags::Color, width, height, m_Desc.Format);
            backBuffer->InitInternal("Swapchain image", imageDesc, image);
            m_Images.push_back(backBuffer);

            Rc backBufferView = ImplCast(pServiceProvider->ResolveRequired<HAL::ImageView>());
            backBufferView->Init(HAL::ImageViewDesc::ForImage(backBuffer.Get(), HAL::ImageAspectFlags::Color));
            m_ImageViews.push_back(backBufferView);
        }

        const auto depthImageDesc = HAL::ImageDesc::Img2D(HAL::ImageBindFlags::Depth, width, height, HAL::Format::D32_SFloat);
        m_DepthImage->Init("Swapchain depth target", depthImageDesc);
        m_DepthImage->AllocateMemory(HAL::MemoryType::DeviceLocal);
        m_DepthImageView = ImplCast(pServiceProvider->ResolveRequired<HAL::ImageView>());
        m_DepthImageView->Init(HAL::ImageViewDesc::ForImage(m_DepthImage.Get(), HAL::ImageAspectFlags::Depth));

        for (size_t i = 0; i < m_Desc.FrameCount; ++i)
        {
            VkSemaphoreCreateInfo semaphoreCI{};
            semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            vkCreateSemaphore(device, &semaphoreCI, VK_NULL_HANDLE, &m_RenderFinishedSemaphores.push_back());
            vkCreateSemaphore(device, &semaphoreCI, VK_NULL_HANDLE, &m_ImageAvailableSemaphores.push_back());
        }

        AcquireNextImage(&m_ImageIndex);
        *m_CurrentImageAvailableSemaphore = m_ImageAvailableSemaphores[m_FrameIndex];
        *m_CurrentRenderFinishedSemaphore = m_RenderFinishedSemaphores[m_FrameIndex];
        return HAL::ResultCode::Success;
    }


    void Swapchain::BuildNativeSwapchain()
    {
#if FE_WINDOWS
        DeviceFactory* pFactory = ImplCast(Env::GetServiceProvider()->ResolveRequired<HAL::DeviceFactory>());

        VkWin32SurfaceCreateInfoKHR surfaceCI{};
        surfaceCI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCI.hinstance = GetModuleHandle(nullptr);
        surfaceCI.hwnd = static_cast<HWND>(m_Desc.NativeWindowHandle);
        vkCreateWin32SurfaceKHR(NativeCast(pFactory), &surfaceCI, VK_NULL_HANDLE, &m_Surface);
#else
#    error platform not supported
#endif

        const VkPhysicalDevice physicalDevice = ImplCast(m_pDevice)->GetNativeAdapter();

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, nullptr);
        festd::small_vector<VkSurfaceFormatKHR> formats(formatCount, VkSurfaceFormatKHR{});
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, formats.data());
        VkBool32 formatSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, m_Queue->GetDesc().QueueFamilyIndex, m_Surface, &formatSupported);
        FE_ASSERT(formatSupported);

        const auto preferredFormat = HAL::Format::B8G8R8A8_SRGB;
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
            FE_LOG_WARNING("Swapchain format {} is not supported, using the first supported one", preferredFormat);
            m_ColorFormat = formats.front();
        }

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &m_Capabilities);
        if (!ValidateDimensions(m_Desc))
        {
            const VkExtent2D min = m_Capabilities.minImageExtent;
            const VkExtent2D max = m_Capabilities.maxImageExtent;
            const uint32_t width = std::clamp(m_Desc.ImageWidth, min.width, max.width);
            const uint32_t height = std::clamp(m_Desc.ImageHeight, min.height, max.height);
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
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &supportedModeCount, nullptr);
            eastl::vector<VkPresentModeKHR> supportedModes(supportedModeCount, VkPresentModeKHR{});
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_Surface, &supportedModeCount, supportedModes.data());
            auto preferredModes = { VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR };

            for (auto& supported : supportedModes)
            {
                for (auto& preferred : preferredModes)
                {
                    if (supported == preferred)
                        mode = supported;
                }
            }
        }

        if (mode == VK_PRESENT_MODE_FIFO_KHR && !m_Desc.VerticalSync)
        {
            FE_LOG_WARNING("V-Sync is force enabled, because FIFO is the only supported present mode");
        }

        VkSwapchainCreateInfoKHR swapchainCI{};
        swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCI.surface = m_Surface;
        swapchainCI.imageArrayLayers = 1;
        swapchainCI.minImageCount = m_Desc.ImageCount;
        swapchainCI.imageExtent = m_Capabilities.currentExtent;
        swapchainCI.imageFormat = m_ColorFormat.format;
        swapchainCI.imageColorSpace = m_ColorFormat.colorSpace;
        swapchainCI.imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCI.presentMode = mode;
        swapchainCI.clipped = true;
        swapchainCI.pQueueFamilyIndices = &m_Queue->GetDesc().QueueFamilyIndex;
        swapchainCI.queueFamilyIndexCount = 1;

        Device* device = ImplCast(m_pDevice);
        vkCreateSwapchainKHR(NativeCast(device), &swapchainCI, VK_NULL_HANDLE, &m_NativeSwapchain);

        // TODO: old hack, must be removed
        // Probably we should use a work batch object that would be submitted to a command queue.
        // Vulkan-specific implementation of such object would contain the semaphores to signal
        // and the semaphores to wait for.
        m_CurrentImageAvailableSemaphore = &device->AddWaitSemaphore();
        m_CurrentRenderFinishedSemaphore = &device->AddSignalSemaphore();
    }


    Swapchain::~Swapchain()
    {
        const VkDevice device = NativeCast(m_pDevice);
        for (auto& semaphore : m_ImageAvailableSemaphores)
        {
            vkDestroySemaphore(device, semaphore, VK_NULL_HANDLE);
        }

        for (auto& semaphore : m_RenderFinishedSemaphores)
        {
            vkDestroySemaphore(device, semaphore, VK_NULL_HANDLE);
        }

        if (m_NativeSwapchain)
            vkDestroySwapchainKHR(device, m_NativeSwapchain, VK_NULL_HANDLE);

        if (m_Surface)
        {
            const VkInstance instance = ImplCast(m_pDevice)->GetDeviceFactory()->GetNative();
            vkDestroySurfaceKHR(instance, m_Surface, VK_NULL_HANDLE);
        }
    }


    const HAL::SwapchainDesc& Swapchain::GetDesc()
    {
        return m_Desc;
    }


    uint32_t Swapchain::GetCurrentImageIndex()
    {
        return m_ImageIndex;
    }


    uint32_t Swapchain::GetImageCount()
    {
        return static_cast<uint32_t>(m_Images.size());
    }


    Image* Swapchain::GetImage(uint32_t index)
    {
        return m_Images[index].Get();
    }


    Image* Swapchain::GetCurrentImage()
    {
        return m_Images[m_ImageIndex].Get();
    }


    void Swapchain::Present()
    {
        ZoneScoped;
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_FrameIndex];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_NativeSwapchain;
        presentInfo.pImageIndices = &m_ImageIndex;
        FE_VK_ASSERT(vkQueuePresentKHR(NativeCast(m_Queue), &presentInfo));

        m_FrameIndex = (m_FrameIndex + 1) % m_Desc.FrameCount;
        *m_CurrentImageAvailableSemaphore = m_ImageAvailableSemaphores[m_FrameIndex];
        *m_CurrentRenderFinishedSemaphore = m_RenderFinishedSemaphores[m_FrameIndex];
        AcquireNextImage(&m_ImageIndex);
    }


    void Swapchain::AcquireNextImage(uint32_t* index)
    {
        const VkSemaphore semaphore = m_ImageAvailableSemaphores[m_FrameIndex];
        const VkDevice device = NativeCast(m_pDevice);
        FE_VK_ASSERT(vkAcquireNextImageKHR(device, m_NativeSwapchain, static_cast<uint64_t>(-1), semaphore, nullptr, index));
    }


    festd::span<HAL::ImageView*> Swapchain::GetRTVs()
    {
        return festd::span(reinterpret_cast<HAL::ImageView**>(m_ImageViews.begin()),
                           reinterpret_cast<HAL::ImageView**>(m_ImageViews.end()));
    }


    HAL::ImageView* Swapchain::GetDSV()
    {
        return m_DepthImageView.Get();
    }


    uint32_t Swapchain::GetCurrentFrameIndex()
    {
        return m_FrameIndex;
    }
} // namespace FE::Graphics::Vulkan
