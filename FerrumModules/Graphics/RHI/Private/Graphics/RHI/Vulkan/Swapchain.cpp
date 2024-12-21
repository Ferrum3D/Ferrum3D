#include <festd/vector.h>
#include <Graphics/RHI/ImageView.h>
#include <Graphics/RHI/Vulkan/CommandQueue.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/DeviceFactory.h>
#include <Graphics/RHI/Vulkan/Fence.h>
#include <Graphics/RHI/Vulkan/Image.h>
#include <Graphics/RHI/Vulkan/ImageView.h>
#include <Graphics/RHI/Vulkan/Swapchain.h>
#include <algorithm>

namespace FE::Graphics::Vulkan
{
    bool Swapchain::ValidateDimensions(const RHI::SwapchainDesc& swapchainDesc) const
    {
        return m_capabilities.minImageExtent.width <= swapchainDesc.m_imageWidth
            && m_capabilities.minImageExtent.height <= swapchainDesc.m_imageHeight
            && m_capabilities.maxImageExtent.width >= swapchainDesc.m_imageWidth
            && m_capabilities.maxImageExtent.height >= swapchainDesc.m_imageHeight;
    }


    Swapchain::Swapchain(RHI::Device* device, Logger* logger, RHI::Image* depthImage)
        : m_logger(logger)
        , m_depthImage(ImplCast(depthImage))
    {
        m_device = device;
    }


    RHI::ResultCode Swapchain::Init(const RHI::SwapchainDesc& desc)
    {
        ZoneScoped;
        m_desc = desc;
        m_queue = ImplCast(desc.m_queue);

        BuildNativeSwapchain();

        const VkDevice device = NativeCast(m_device);
        vkGetSwapchainImagesKHR(device, m_nativeSwapchain, &m_desc.m_frameCount, nullptr);
        festd::small_vector<VkImage> images(m_desc.m_frameCount, VK_NULL_HANDLE);
        vkGetSwapchainImagesKHR(device, m_nativeSwapchain, &m_desc.m_frameCount, images.data());

        DI::IServiceProvider* pServiceProvider = Env::GetServiceProvider();

        const uint32_t width = m_desc.m_imageWidth;
        const uint32_t height = m_desc.m_imageHeight;
        m_desc.m_format = VKConvert(m_colorFormat.format);
        for (auto& image : images)
        {
            Rc backBuffer = ImplCast(pServiceProvider->ResolveRequired<RHI::Image>());
            const auto imageDesc = RHI::ImageDesc::Img2D(RHI::ImageBindFlags::kColor, width, height, m_desc.m_format);
            backBuffer->InitInternal("Swapchain image", imageDesc, image);
            m_images.push_back(backBuffer);

            Rc backBufferView = ImplCast(pServiceProvider->ResolveRequired<RHI::ImageView>());
            backBufferView->Init(RHI::ImageViewDesc::ForImage(backBuffer.Get(), RHI::ImageAspectFlags::kColor));
            m_imageViews.push_back(backBufferView);
        }

        const auto depthImageDesc = RHI::ImageDesc::Img2D(RHI::ImageBindFlags::kDepth, width, height, RHI::Format::kD32_SFLOAT);
        m_depthImage->Init("Swapchain depth target", depthImageDesc);
        m_depthImage->AllocateMemory(RHI::MemoryType::kDeviceLocal);
        m_depthImageView = ImplCast(pServiceProvider->ResolveRequired<RHI::ImageView>());
        m_depthImageView->Init(RHI::ImageViewDesc::ForImage(m_depthImage.Get(), RHI::ImageAspectFlags::kDepth));

        VkSemaphoreCreateInfo semaphoreCI{};
        semaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        m_renderFinishedSemaphores.resize(m_desc.m_frameCount);
        m_imageAvailableSemaphores.resize(m_desc.m_frameCount);

        for (uint32_t frameIndex = 0; frameIndex < m_desc.m_frameCount; ++frameIndex)
        {
            vkCreateSemaphore(device, &semaphoreCI, nullptr, &m_renderFinishedSemaphores[frameIndex]);
            vkCreateSemaphore(device, &semaphoreCI, nullptr, &m_imageAvailableSemaphores[frameIndex]);
        }

        return RHI::ResultCode::kSuccess;
    }


    void Swapchain::BuildNativeSwapchain()
    {
#if FE_PLATFORM_WINDOWS
        DeviceFactory* factory = ImplCast(Env::GetServiceProvider()->ResolveRequired<RHI::DeviceFactory>());

        VkWin32SurfaceCreateInfoKHR surfaceCI{};
        surfaceCI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCI.hinstance = GetModuleHandle(nullptr);
        surfaceCI.hwnd = static_cast<HWND>(m_desc.m_nativeWindowHandle);
        vkCreateWin32SurfaceKHR(NativeCast(factory), &surfaceCI, nullptr, &m_surface);
#else
#    error platform not supported
#endif

        const VkPhysicalDevice physicalDevice = ImplCast(m_device)->GetNativeAdapter();

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, nullptr);
        festd::small_vector<VkSurfaceFormatKHR> formats(formatCount, VkSurfaceFormatKHR{});
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, formats.data());
        VkBool32 formatSupported;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, m_queue->GetDesc().m_queueFamilyIndex, m_surface, &formatSupported);
        FE_Assert(formatSupported);

        const auto preferredFormat = RHI::Format::kB8G8R8A8_SRGB;
        m_colorFormat.format = VK_FORMAT_UNDEFINED;
        for (auto& fmt : formats)
        {
            if (fmt.format == preferredFormat && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                m_colorFormat = fmt;
            }
        }
        if (m_colorFormat.format == VK_FORMAT_UNDEFINED)
        {
            m_logger->LogWarning("Swapchain format {} is not supported, using the first supported one", preferredFormat);
            m_colorFormat = formats.front();
        }

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &m_capabilities);
        if (!ValidateDimensions(m_desc))
        {
            const VkExtent2D min = m_capabilities.minImageExtent;
            const VkExtent2D max = m_capabilities.maxImageExtent;
            const uint32_t width = std::clamp(m_desc.m_imageWidth, min.width, max.width);
            const uint32_t height = std::clamp(m_desc.m_imageHeight, min.height, max.height);
            m_logger->LogWarning("Requested swap chain size ({}, {}) was resized to ({}, {}) according to capabilities",
                                 m_desc.m_imageWidth,
                                 m_desc.m_imageHeight,
                                 width,
                                 height);
            m_desc.m_imageWidth = width;
            m_desc.m_imageHeight = height;
        }

        VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;

        // If v-sync is disabled, try to use either immediate or mailbox (if supported).
        if (!m_desc.m_verticalSync)
        {
            uint32_t supportedModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &supportedModeCount, nullptr);
            eastl::vector<VkPresentModeKHR> supportedModes(supportedModeCount, VkPresentModeKHR{});
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &supportedModeCount, supportedModes.data());
            const std::array preferredModes = { VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR };

            for (const VkPresentModeKHR supported : supportedModes)
            {
                for (const VkPresentModeKHR preferred : preferredModes)
                {
                    if (supported == preferred)
                        mode = supported;
                }
            }
        }

        if (mode == VK_PRESENT_MODE_FIFO_KHR && !m_desc.m_verticalSync)
        {
            m_logger->LogWarning("V-Sync is force enabled, because FIFO is the only supported present mode");
        }

        VkSwapchainCreateInfoKHR swapchainCI{};
        swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCI.surface = m_surface;
        swapchainCI.imageArrayLayers = 1;
        swapchainCI.minImageCount = m_desc.m_frameCount;
        swapchainCI.imageExtent = m_capabilities.currentExtent;
        swapchainCI.imageFormat = m_colorFormat.format;
        swapchainCI.imageColorSpace = m_colorFormat.colorSpace;
        swapchainCI.imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainCI.presentMode = mode;
        swapchainCI.clipped = true;
        swapchainCI.pQueueFamilyIndices = &m_queue->GetDesc().m_queueFamilyIndex;
        swapchainCI.queueFamilyIndexCount = 1;

        vkCreateSwapchainKHR(NativeCast(m_device), &swapchainCI, VK_NULL_HANDLE, &m_nativeSwapchain);
    }


    Swapchain::~Swapchain()
    {
        const VkDevice device = NativeCast(m_device);

        for (VkSemaphore semaphore : m_imageAvailableSemaphores)
            vkDestroySemaphore(device, semaphore, VK_NULL_HANDLE);

        for (VkSemaphore semaphore : m_renderFinishedSemaphores)
            vkDestroySemaphore(device, semaphore, VK_NULL_HANDLE);

        if (m_nativeSwapchain)
            vkDestroySwapchainKHR(device, m_nativeSwapchain, VK_NULL_HANDLE);

        if (m_surface)
        {
            const VkInstance instance = ImplCast(m_device)->GetDeviceFactory()->GetNative();
            vkDestroySurfaceKHR(instance, m_surface, VK_NULL_HANDLE);
        }
    }


    const RHI::SwapchainDesc& Swapchain::GetDesc() const
    {
        return m_desc;
    }


    uint32_t Swapchain::GetCurrentImageIndex() const
    {
        return m_imageIndex;
    }


    uint32_t Swapchain::GetImageCount() const
    {
        return static_cast<uint32_t>(m_images.size());
    }


    void Swapchain::BeginFrame(const RHI::FenceSyncPoint& signalFence)
    {
        ZoneScoped;

        const uint64_t tempValue = UINT64_MAX;
        const VkPipelineStageFlags kWaitDstFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;

        const VkSemaphore imageAvailableSemaphore = m_imageAvailableSemaphores[m_frameIndex];
        const VkDevice device = NativeCast(m_device);
        vkAcquireNextImageKHR(device, m_nativeSwapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &m_imageIndex);

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
        signalSubmitInfo.pWaitSemaphores = &imageAvailableSemaphore;
        signalSubmitInfo.signalSemaphoreCount = 1;
        signalSubmitInfo.pSignalSemaphores = &vkSignalFence;
        signalSubmitInfo.pWaitDstStageMask = &kWaitDstFlags;
        FE_VK_ASSERT(vkQueueSubmit(NativeCast(m_queue), 1, &signalSubmitInfo, VK_NULL_HANDLE));
    }


    void Swapchain::Present(const RHI::FenceSyncPoint& waitFence)
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
        signalSubmitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_frameIndex];
        signalSubmitInfo.pWaitDstStageMask = &kWaitDstFlags;
        FE_VK_ASSERT(vkQueueSubmit(NativeCast(m_queue), 1, &signalSubmitInfo, VK_NULL_HANDLE));

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_frameIndex];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_nativeSwapchain;
        presentInfo.pImageIndices = &m_imageIndex;
        FE_VK_ASSERT(vkQueuePresentKHR(NativeCast(m_queue), &presentInfo));

        m_frameIndex = (m_frameIndex + 1) % m_desc.m_frameCount;
    }


    festd::span<RHI::ImageView* const> Swapchain::GetRTVs() const
    {
        return festd::span(reinterpret_cast<RHI::ImageView* const*>(m_imageViews.begin().base()),
                           reinterpret_cast<RHI::ImageView* const*>(m_imageViews.end().base()));
    }


    RHI::ImageView* Swapchain::GetDSV() const
    {
        return m_depthImageView.Get();
    }
} // namespace FE::Graphics::Vulkan
