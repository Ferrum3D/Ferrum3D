#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/DeviceFactory.h>
#include <Graphics/Core/Vulkan/FrameGraph/FrameGraphContext.h>
#include <Graphics/Core/Vulkan/Platform/VulkanSurface.h>
#include <Graphics/Core/Vulkan/RenderTarget.h>
#include <Graphics/Core/Vulkan/Viewport.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        festd::small_vector<VkSurfaceFormatKHR> EnumerateSurfaceFormats(const VkPhysicalDevice physicalDevice,
                                                                        const VkSurfaceKHR surface)
        {
            FE_PROFILER_ZONE();

            uint32_t formatCount = 0;
            VerifyVulkan(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));

            festd::small_vector<VkSurfaceFormatKHR> formats{ formatCount };
            VerifyVulkan(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data()));
            return formats;
        }


        festd::small_vector<VkPresentModeKHR> EnumeratePresentModes(const VkPhysicalDevice physicalDevice,
                                                                    const VkSurfaceKHR surface)
        {
            FE_PROFILER_ZONE();

            uint32_t modeCount = 0;
            VerifyVulkan(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, nullptr));

            festd::small_vector<VkPresentModeKHR> modes{ modeCount };
            VerifyVulkan(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, modes.data()));
            return modes;
        }


        VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
        {
            FE_PROFILER_ZONE();

            VkSurfaceCapabilitiesKHR capabilities;
            VerifyVulkan(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities));
            return capabilities;
        }


        festd::string_view TransformFlagToString(const VkSurfaceTransformFlagsKHR flag)
        {
            switch (flag)
            {
            case 0:
            case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:
                return "Identity";
            case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
                return "Rot90";
            case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
                return "Rot180";
            case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
                return "Rot270";
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR:
                return "HrzMir";
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:
                return "HrzMirRot90";
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR:
                return "HrzMirRot180";
            case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR:
                return "HrzMirRot270";
            case VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR:
                return "Inherit";
            default:
                return "Unknown";
            }
        }


        festd::fixed_string ToString(const VkSurfaceTransformFlagBitsKHR transform)
        {
            festd::fixed_string result;
            Bit::Traverse(static_cast<uint32_t>(transform), [&result](const uint32_t bit) {
                if (!result.empty())
                    result += "|";
                result += TransformFlagToString(bit);
            });

            return result;
        }


        festd::string_view ToString(const VkPresentModeKHR mode)
        {
            switch (mode)
            {
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                return "Immediate";
            case VK_PRESENT_MODE_MAILBOX_KHR:
                return "Mailbox";
            case VK_PRESENT_MODE_FIFO_KHR:
                return "FIFO";
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                return "FIFO Relaxed";
            default:
                return "Unknown";
            }
        }


        void ImageMemoryBarrier(const VkCommandBuffer cmdbuffer, const VkImage image, const VkAccessFlags srcAccessMask,
                                const VkAccessFlags dstAccessMask, const VkImageLayout oldImageLayout,
                                const VkImageLayout newImageLayout, const VkPipelineStageFlags srcStageMask,
                                const VkPipelineStageFlags dstStageMask, const VkImageAspectFlags aspect)
        {
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = srcAccessMask;
            imageMemoryBarrier.dstAccessMask = dstAccessMask;
            imageMemoryBarrier.oldLayout = oldImageLayout;
            imageMemoryBarrier.newLayout = newImageLayout;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = { aspect, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }


        constexpr VkFormat kPreferredSwapchainColorFormats[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM };
        constexpr VkPresentModeKHR kPreferredSwapchainPresentModes[] = { VK_PRESENT_MODE_FIFO_RELAXED_KHR,
                                                                         VK_PRESENT_MODE_FIFO_KHR };
    } // namespace


    Viewport::Viewport(Core::Device* device, Logger* logger, Core::ResourcePool* resourcePool)
        : m_logger(logger)
        , m_resourcePool(resourcePool)
    {
        FE_PROFILER_ZONE();

        m_device = device;
        m_deviceFactory = ImplCast(device)->GetDeviceFactory();

        const uint32_t graphicsFamily = ImplCast(device)->GetQueueFamilyIndex(Core::HardwareQueueKindFlags::kGraphics);

        vkGetDeviceQueue(NativeCast(device), graphicsFamily, 0, &m_queue);
    }


    Viewport::~Viewport()
    {
        ReleaseResources();

        m_imageAvailableSemaphores.clear();
        m_renderFinishedSemaphores.clear();
        m_graphicsCommandBuffers.clear();
    }


    void Viewport::Init(const Core::ViewportDesc& desc)
    {
        FE_PROFILER_ZONE();

        m_desc = desc;

        m_logger->LogDebug("Creating Vulkan swapchain");

        for (uint32_t i = 0; i < kMaxInFlightFrames; ++i)
        {
            m_imageAvailableSemaphores.push_back(Semaphore::Create(m_device, Fmt::FormatName("ImageAvailableSemaphore_{}", i)));
            m_renderFinishedSemaphores.push_back(Semaphore::Create(m_device, Fmt::FormatName("RenderFinishedSemaphore_{}", i)));

            const Env::Name name = Fmt::FormatName("GraphicsCommandBuffer_{}", i);
            m_graphicsCommandBuffers.push_back(CommandBuffer::Create(m_device, name, Core::HardwareQueueKindFlags::kGraphics));
        }

        m_fence = Fence::Create(m_device);
        m_fence->Init(0);

        CreateSurface();
        CreateSwapChain();
        CreateResources();
    }


    const Core::ViewportDesc& Viewport::GetDesc() const
    {
        return m_desc;
    }


    void Viewport::PrepareFrame()
    {
        FE_PROFILER_ZONE();

        if (m_frameIndex > kMaxInFlightFrames)
        {
            m_fence->Wait(m_frameIndex - kMaxInFlightFrames);
        }

        AcquireNextImage();

        Image* colorTarget = m_renderTargets[m_imageIndex].Get();
        Image* depthStencil = m_depthTarget.Get();

        const VkCommandBuffer commandBuffer = GetCurrentGraphicsCommandBuffer()->GetNative();

        VerifyVulkan(vkResetCommandBuffer(commandBuffer, 0));

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VerifyVulkan(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        ImageMemoryBarrier(commandBuffer,
                           colorTarget->GetNative(),
                           0,
                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                           VK_IMAGE_ASPECT_COLOR_BIT);
        ImageMemoryBarrier(commandBuffer,
                           depthStencil->GetNative(),
                           0,
                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                           VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    }


    void Viewport::AcquireNextImage()
    {
        FE_PROFILER_ZONE();

        if (m_frameIndex > kMaxInFlightFrames)
        {
            m_fence->Wait(m_frameIndex - kMaxInFlightFrames);
        }

        const uint32_t semaphoreIndex = m_frameIndex % kMaxInFlightFrames;
        Semaphore* semaphore = m_imageAvailableSemaphores[semaphoreIndex].Get();
        const VkResult result = vkAcquireNextImageKHR(
            NativeCast(m_device), m_swapchain, Constants::kMaxU64, semaphore->GetNative(), VK_NULL_HANDLE, &m_imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain();
        }
        else if (result != VK_SUBOPTIMAL_KHR)
        {
            VerifyVulkan(result);
        }
    }


    void Viewport::CreateSurface()
    {
        FE_PROFILER_ZONE();

        const VkInstance vkInstance = m_deviceFactory->GetNative();

        if (m_surface)
        {
            vkDestroySurfaceKHR(vkInstance, m_surface, nullptr);
            m_surface = VK_NULL_HANDLE;
        }

        VerifyVulkan(Vulkan::CreateSurface(vkInstance, m_desc.m_nativeWindowHandle, &m_surface));
    }


    void Viewport::CreateSwapChain()
    {
        FE_PROFILER_ZONE();

        const Device* device = ImplCast(m_device);
        const VkPhysicalDevice physicalDevice = device->GetNativeAdapter();
        const auto surfaceFormats = EnumerateSurfaceFormats(physicalDevice, m_surface);
        const VkSurfaceCapabilitiesKHR capabilities = GetSurfaceCapabilities(physicalDevice, m_surface);
        const VkExtent2D minExtent = capabilities.minImageExtent;
        const VkExtent2D maxExtent = capabilities.maxImageExtent;

        const VkSwapchainKHR oldSwapchain = m_swapchain;
        m_swapchain = VK_NULL_HANDLE;

        m_desc.m_width = Math::Clamp(m_desc.m_width, minExtent.width, maxExtent.width);
        m_desc.m_height = Math::Clamp(m_desc.m_height, minExtent.height, maxExtent.height);

        VkSwapchainCreateInfoKHR swapchainCI = {};
        swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCI.oldSwapchain = oldSwapchain;
        swapchainCI.surface = m_surface;
        swapchainCI.imageFormat = VK_FORMAT_UNDEFINED;
        swapchainCI.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainCI.imageExtent.width = m_desc.m_width;
        swapchainCI.imageExtent.height = m_desc.m_height;
        swapchainCI.imageArrayLayers = 1;
        swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCI.preTransform = capabilities.currentTransform;
        swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCI.pQueueFamilyIndices = nullptr;
        swapchainCI.queueFamilyIndexCount = 0;
        swapchainCI.clipped = VK_TRUE;

        FE_Assert(capabilities.maxImageCount >= kMaxInFlightFrames);
        FE_Assert(capabilities.minImageCount <= kMaxInFlightFrames);
        swapchainCI.minImageCount = kMaxInFlightFrames;

        m_logger->LogTrace("Swapchain image count: {}", swapchainCI.minImageCount);

        bool formatSelected = false;
        for (const VkFormat requestedFormat : kPreferredSwapchainColorFormats)
        {
            for (const auto [format, colorSpace] : surfaceFormats)
            {
                if (format == requestedFormat)
                {
                    swapchainCI.imageFormat = format;
                    swapchainCI.imageColorSpace = colorSpace;
                    formatSelected = true;
                    break;
                }
            }

            if (formatSelected)
                break;
        }

        if (swapchainCI.imageFormat == VK_FORMAT_UNDEFINED)
        {
            m_logger->LogCritical("Failed to find suitable Vulkan swapchain color format");
            FE_DebugBreak();
        }

        m_rtvFormat = Translate(swapchainCI.imageFormat);
        m_logger->LogTrace("Swapchain color format: {}", ToString(m_rtvFormat));
        m_logger->LogTrace("Swapchain pre-transform: {}", ToString(swapchainCI.preTransform));

        const auto presentModes = EnumeratePresentModes(physicalDevice, m_surface);
        for (const auto presentMode : kPreferredSwapchainPresentModes)
        {
            for (const VkPresentModeKHR mode : presentModes)
            {
                if (mode == presentMode)
                {
                    swapchainCI.presentMode = mode;
                    break;
                }
            }
        }

        m_logger->LogTrace("Swapchain present mode: {}", ToString(swapchainCI.presentMode));

        const VkDevice vkDevice = device->GetNative();
        VerifyVulkan(vkCreateSwapchainKHR(vkDevice, &swapchainCI, nullptr, &m_swapchain));

        if (oldSwapchain)
        {
            vkDestroySwapchainKHR(vkDevice, oldSwapchain, nullptr);
        }
    }


    void Viewport::CreateResources()
    {
        FE_PROFILER_ZONE();

        const VkDevice vkDevice = NativeCast(m_device);

        const uint32_t width = m_desc.m_width;
        const uint32_t height = m_desc.m_height;

        uint32_t imageCount = 0;
        VerifyVulkan(vkGetSwapchainImagesKHR(vkDevice, m_swapchain, &imageCount, nullptr));

        m_logger->LogDebug("Created Vulkan swapchain with {} images", imageCount);

        festd::small_vector<VkImage> images{ imageCount };
        VerifyVulkan(vkGetSwapchainImagesKHR(vkDevice, m_swapchain, &imageCount, images.data()));

        FE_Assert(m_imageAvailableSemaphores.size() == kMaxInFlightFrames);
        FE_Assert(m_renderFinishedSemaphores.size() == kMaxInFlightFrames);
        FE_Assert(m_renderTargets.empty());

        const Core::ImageDesc colorTargetDesc = Core::ImageDesc::Img2D(width, height, m_rtvFormat);

        for (uint32_t i = 0; i < imageCount; ++i)
        {
            const Env::Name imageName = Fmt::FormatName("Swapchain Color Target {}", i);

            const Rc image = RenderTarget::Create(m_device);
            image->InitInternal(imageName, colorTargetDesc, images[i]);
            image->SetImmediateDestroyPolicy();

            VkDebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(image->GetNative());
            nameInfo.pObjectName = imageName.c_str();
            VerifyVulkan(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));

            m_renderTargets.push_back(image);
        }

        const Core::ImageDesc depthTargetDesc = Core::ImageDesc::Img2D(width, height, Core::kMainDepthTargetFormat);
        m_depthTarget = ImplCast(m_resourcePool->CreateRenderTarget("Swapchain Depth Stencil Target", depthTargetDesc));
        m_depthTarget->SetImmediateDestroyPolicy();
    }


    void Viewport::ReleaseResources()
    {
        FE_PROFILER_ZONE();

        m_device->WaitIdle();

        if (m_frameIndex > 1)
        {
            m_fence->Wait(m_frameIndex - 1);
        }

        m_renderTargets.clear();
        m_depthTarget.Reset();
    }


    void Viewport::RecreateSwapchain()
    {
        FE_PROFILER_ZONE();

        ReleaseResources();

        // CreateSurface();
        CreateSwapChain();
        CreateResources();
    }


    void Viewport::Resize(const uint32_t width, const uint32_t height)
    {
        FE_PROFILER_ZONE();

        m_desc.m_width = width;
        m_desc.m_height = height;

        RecreateSwapchain();
    }


    Core::RenderTarget* Viewport::GetCurrentColorTarget()
    {
        return m_renderTargets[m_imageIndex].Get();
    }


    Core::RenderTarget* Viewport::GetDepthTarget()
    {
        return m_depthTarget.Get();
    }


    Core::Format Viewport::GetColorTargetFormat()
    {
        return m_rtvFormat;
    }


    Core::Format Viewport::GetDepthTargetFormat()
    {
        return Core::kMainDepthTargetFormat;
    }


    void Viewport::Present(FrameGraphContext* frameGraphContext)
    {
        FE_PROFILER_ZONE();

        Image* renderTarget = m_renderTargets[m_imageIndex].Get();

        ImageMemoryBarrier(frameGraphContext->m_graphicsCommandBuffer->GetNative(),
                           renderTarget->GetNative(),
                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                           0,
                           VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                           0,
                           VK_IMAGE_ASPECT_COLOR_BIT);

        const uint32_t semaphoreIndex = m_frameIndex % kMaxInFlightFrames;
        const VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
        frameGraphContext->EnqueueSemaphoreToWait(m_imageAvailableSemaphores[semaphoreIndex].Get(), stageFlags);
        frameGraphContext->EnqueueSemaphoreToSignal(m_renderFinishedSemaphores[semaphoreIndex].Get());
        frameGraphContext->EnqueueFenceToSignal({ m_fence, m_frameIndex++ });

        frameGraphContext->Submit();

        const VkSemaphore waitSemaphore = m_renderFinishedSemaphores[semaphoreIndex]->GetNative();

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_swapchain;
        presentInfo.pImageIndices = &m_imageIndex;

        const VkResult presentResult = vkQueuePresentKHR(m_queue, &presentInfo);
        if (presentResult == VK_SUBOPTIMAL_KHR || presentResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain();
        }
        else
        {
            VerifyVulkan(presentResult);
        }
    }
} // namespace FE::Graphics::Vulkan
