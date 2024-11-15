#include <FeCore/Containers/SmallVector.h>
#include <HAL/ImageView.h>
#include <HAL/Vulkan/CommandQueue.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/DeviceFactory.h>
#include <HAL/Vulkan/Fence.h>
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


    Swapchain::Swapchain(HAL::Device* pDevice, Logger* logger, HAL::Image* pDepthImage)
        : m_logger(logger)
        , m_DepthImage(ImplCast(pDepthImage))
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
        vkGetSwapchainImagesKHR(device, m_NativeSwapchain, &m_Desc.FrameCount, nullptr);
        festd::small_vector<VkImage> images(m_Desc.FrameCount, VK_NULL_HANDLE);
        vkGetSwapchainImagesKHR(device, m_NativeSwapchain, &m_Desc.FrameCount, images.data());

        DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();

        const uint32_t width = m_Desc.ImageWidth;
        const uint32_t height = m_Desc.ImageHeight;
        m_Desc.Format = VKConvert(m_ColorFormat.format);
        for (auto& image : images)
        {
            Rc backBuffer = ImplCast(pServiceProvider->ResolveRequired<HAL::Image>());
            const auto imageDesc = HAL::ImageDesc::Img2D(HAL::ImageBindFlags::kColor, width, height, m_Desc.Format);
            backBuffer->InitInternal("Swapchain image", imageDesc, image);
            m_Images.push_back(backBuffer);

            Rc backBufferView = ImplCast(pServiceProvider->ResolveRequired<HAL::ImageView>());
            backBufferView->Init(HAL::ImageViewDesc::ForImage(backBuffer.Get(), HAL::ImageAspectFlags::kColor));
            m_ImageViews.push_back(backBufferView);
        }

        const auto depthImageDesc = HAL::ImageDesc::Img2D(HAL::ImageBindFlags::kDepth, width, height, HAL::Format::kD32_SFLOAT);
        m_DepthImage->Init("Swapchain depth target", depthImageDesc);
        m_DepthImage->AllocateMemory(HAL::MemoryType::kDeviceLocal);
        m_DepthImageView = ImplCast(pServiceProvider->ResolveRequired<HAL::ImageView>());
        m_DepthImageView->Init(HAL::ImageViewDesc::ForImage(m_DepthImage.Get(), HAL::ImageAspectFlags::kDepth));

        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(device, &semaphoreCI, nullptr, &m_RenderFinishedSemaphore);
        vkCreateSemaphore(device, &semaphoreCI, nullptr, &m_ImageAvailableSemaphore);

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
        vkCreateWin32SurfaceKHR(NativeCast(pFactory), &surfaceCI, nullptr, &m_Surface);
#else
#    error platform not supported
#endif

        const VkPhysicalDevice physicalDevice = ImplCast(m_pDevice)->GetNativeAdapter();

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, nullptr);
        festd::small_vector<VkSurfaceFormatKHR> formats(formatCount, VkSurfaceFormatKHR{});
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, formats.data());
        VkBool32 formatSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, m_Queue->GetDesc().m_queueFamilyIndex, m_Surface, &formatSupported);
        FE_Assert(formatSupported);

        const auto preferredFormat = HAL::Format::kB8G8R8A8_SRGB;
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
            m_logger->LogWarning("Swapchain format {} is not supported, using the first supported one", preferredFormat);
            m_ColorFormat = formats.front();
        }

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &m_Capabilities);
        if (!ValidateDimensions(m_Desc))
        {
            const VkExtent2D min = m_Capabilities.minImageExtent;
            const VkExtent2D max = m_Capabilities.maxImageExtent;
            const uint32_t width = std::clamp(m_Desc.ImageWidth, min.width, max.width);
            const uint32_t height = std::clamp(m_Desc.ImageHeight, min.height, max.height);
            m_logger->LogWarning("Requested swap chain size ({}, {}) was resized to ({}, {}) according to capabilities",
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
            m_logger->LogWarning("V-Sync is force enabled, because FIFO is the only supported present mode");
        }

        VkSwapchainCreateInfoKHR swapchainCI{};
        swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCI.surface = m_Surface;
        swapchainCI.imageArrayLayers = 1;
        swapchainCI.minImageCount = m_Desc.FrameCount;
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
        swapchainCI.pQueueFamilyIndices = &m_Queue->GetDesc().m_queueFamilyIndex;
        swapchainCI.queueFamilyIndexCount = 1;

        vkCreateSwapchainKHR(NativeCast(m_pDevice), &swapchainCI, VK_NULL_HANDLE, &m_NativeSwapchain);
    }


    Swapchain::~Swapchain()
    {
        const VkDevice device = NativeCast(m_pDevice);

        if (m_ImageAvailableSemaphore)
            vkDestroySemaphore(device, m_ImageAvailableSemaphore, VK_NULL_HANDLE);

        if (m_RenderFinishedSemaphore)
            vkDestroySemaphore(device, m_RenderFinishedSemaphore, VK_NULL_HANDLE);

        if (m_NativeSwapchain)
            vkDestroySwapchainKHR(device, m_NativeSwapchain, VK_NULL_HANDLE);

        if (m_Surface)
        {
            const VkInstance instance = ImplCast(m_pDevice)->GetDeviceFactory()->GetNative();
            vkDestroySurfaceKHR(instance, m_Surface, VK_NULL_HANDLE);
        }
    }


    const HAL::SwapchainDesc& Swapchain::GetDesc() const
    {
        return m_Desc;
    }


    uint32_t Swapchain::GetCurrentImageIndex() const
    {
        return m_FrameIndex;
    }


    uint32_t Swapchain::GetImageCount() const
    {
        return static_cast<uint32_t>(m_Images.size());
    }


    void Swapchain::BeginFrame(const HAL::FenceSyncPoint& signalFence)
    {
        ZoneScoped;

        const uint64_t tempValue = UINT64_MAX;
        const VkPipelineStageFlags kWaitDstFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

        const VkDevice device = NativeCast(m_pDevice);
        vkAcquireNextImageKHR(device, m_NativeSwapchain, UINT64_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &m_FrameIndex);

        VkTimelineSemaphoreSubmitInfo timelineSemaphoreInfo{};
        timelineSemaphoreInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineSemaphoreInfo.waitSemaphoreValueCount = 1;
        timelineSemaphoreInfo.pWaitSemaphoreValues = &tempValue;
        timelineSemaphoreInfo.signalSemaphoreValueCount = 1;
        timelineSemaphoreInfo.pSignalSemaphoreValues = &signalFence.m_value;

        const VkSemaphore vkSignalFence = NativeCast(signalFence.m_fence.Get());

        VkSubmitInfo signalSubmitInfo{};
        signalSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        signalSubmitInfo.pNext = &timelineSemaphoreInfo;
        signalSubmitInfo.waitSemaphoreCount = 1;
        signalSubmitInfo.pWaitSemaphores = &m_ImageAvailableSemaphore;
        signalSubmitInfo.signalSemaphoreCount = 1;
        signalSubmitInfo.pSignalSemaphores = &vkSignalFence;
        signalSubmitInfo.pWaitDstStageMask = &kWaitDstFlags;
        FE_VK_ASSERT(vkQueueSubmit(NativeCast(m_Queue), 1, &signalSubmitInfo, VK_NULL_HANDLE));
    }


    void Swapchain::Present(const HAL::FenceSyncPoint& waitFence)
    {
        ZoneScoped;

        const uint64_t tempValue = UINT64_MAX;
        const VkPipelineStageFlags kWaitDstFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

        VkTimelineSemaphoreSubmitInfo timelineSemaphoreInfo{};
        timelineSemaphoreInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineSemaphoreInfo.waitSemaphoreValueCount = 1;
        timelineSemaphoreInfo.pWaitSemaphoreValues = &waitFence.m_value;
        timelineSemaphoreInfo.signalSemaphoreValueCount = 1;
        timelineSemaphoreInfo.pSignalSemaphoreValues = &tempValue;

        const VkSemaphore vkWaitFence = NativeCast(waitFence.m_fence.Get());

        VkSubmitInfo signalSubmitInfo{};
        signalSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        signalSubmitInfo.pNext = &timelineSemaphoreInfo;
        signalSubmitInfo.waitSemaphoreCount = 1;
        signalSubmitInfo.pWaitSemaphores = &vkWaitFence;
        signalSubmitInfo.signalSemaphoreCount = 1;
        signalSubmitInfo.pSignalSemaphores = &m_RenderFinishedSemaphore;
        signalSubmitInfo.pWaitDstStageMask = &kWaitDstFlags;
        FE_VK_ASSERT(vkQueueSubmit(NativeCast(m_Queue), 1, &signalSubmitInfo, VK_NULL_HANDLE));

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_NativeSwapchain;
        presentInfo.pImageIndices = &m_FrameIndex;
        FE_VK_ASSERT(vkQueuePresentKHR(NativeCast(m_Queue), &presentInfo));
    }


    festd::span<HAL::ImageView* const> Swapchain::GetRTVs() const
    {
        return festd::span(reinterpret_cast<HAL::ImageView* const*>(m_ImageViews.begin().base()),
                           reinterpret_cast<HAL::ImageView* const*>(m_ImageViews.end().base()));
    }


    HAL::ImageView* Swapchain::GetDSV() const
    {
        return m_DepthImageView.Get();
    }
} // namespace FE::Graphics::Vulkan
