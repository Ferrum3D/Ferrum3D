#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/GpuProfiler.h>

namespace FE::Graphics::Vulkan
{
    GpuProfiler::~GpuProfiler()
    {
#if defined(TRACY_ENABLE)
        FE_Assert(m_context == nullptr, "Shut down GPU profiling after joining queue work");
#endif
    }


    void GpuProfiler::Init([[maybe_unused]] Core::Device* device, [[maybe_unused]] const Core::DeviceQueueType queueType,
                           [[maybe_unused]] const VkQueue queue, [[maybe_unused]] const char* name)
    {
#if defined(TRACY_ENABLE)
        FE_Assert(m_context == nullptr);

        const auto* vkDevice = ImplCast(device);
        const uint32_t familyIndex = vkDevice->GetQueueFamilyIndex(queueType);
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(vkDevice->GetNativeAdapter(), &familyCount, nullptr);
        festd::inline_vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(vkDevice->GetNativeAdapter(), &familyCount, families.data());
        if (families[familyIndex].timestampValidBits == 0)
        {
            Logger::LogWarning("GPU profiling is unavailable for {}: queue timestamps are unsupported", name);
            return;
        }

        m_usesHostQueryReset = (families[familyIndex].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == 0;
        if (m_usesHostQueryReset)
        {
            VkPhysicalDeviceVulkan12Features supported12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
            VkPhysicalDeviceFeatures2 supported{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &supported12 };
            vkGetPhysicalDeviceFeatures2(vkDevice->GetNativeAdapter(), &supported);
            if (!supported12.hostQueryReset)
            {
                Logger::LogWarning("GPU profiling is unavailable for {}: host query resets are unsupported", name);
                return;
            }
        }

        m_device = vkDevice->GetNative();
        m_queue = queue;
        VkCommandPoolCreateInfo poolInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = familyIndex;
        VerifyVk(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool));

        VkCommandBufferAllocateInfo allocateInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocateInfo.commandPool = m_commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;
        VerifyVk(vkAllocateCommandBuffers(m_device, &allocateInfo, &m_collectionCommands));

        VkFenceCreateInfo fenceInfo{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VerifyVk(vkCreateFence(m_device, &fenceInfo, nullptr, &m_collectionFence));
        m_context = tracy::CreateVkContext(vkDevice->GetNativeAdapter(),
                                           m_device,
                                           queue,
                                           m_collectionCommands,
                                           nullptr,
                                           nullptr,
                                           m_usesHostQueryReset ? vkResetQueryPool : nullptr);
        m_remainingHostQueries = m_context->GetQueryCount();
        TracyVkContextName(m_context, name, static_cast<uint16_t>(strlen(name)));
#endif
    }


    void GpuProfiler::Shutdown()
    {
#if defined(TRACY_ENABLE)
        if (m_context == nullptr)
            return;

        FE_Assert(m_zones.empty());
        VerifyVk(vkQueueWaitIdle(m_queue));
        CollectIdle();
        VerifyVk(vkQueueWaitIdle(m_queue));
        TracyVkDestroy(m_context);
        m_context = nullptr;
        vkDestroyFence(m_device, m_collectionFence, nullptr);
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
#endif
    }


    void GpuProfiler::BeginZone([[maybe_unused]] const VkCommandBuffer commandBuffer, [[maybe_unused]] const char* name,
                                [[maybe_unused]] const SourceLocation location)
    {
#if defined(TRACY_ENABLE)
        if (m_context == nullptr)
            return;

        // Keep streaming nonblocking if the query ring fills before the transfer queue becomes idle.
        const bool active = !m_usesHostQueryReset || m_remainingHostQueries >= 2;
        if (m_usesHostQueryReset && active)
            m_remainingHostQueries -= 2;

        m_zones.emplace_back(m_context,
                             location.m_lineNumber,
                             location.m_fileName,
                             strlen(location.m_fileName),
                             TracyFunction,
                             strlen(TracyFunction),
                             name,
                             strlen(name),
                             commandBuffer,
                             active,
                             uint32_t{ 0 });
        m_hasUncollectedZones |= active;
#endif
    }


    void GpuProfiler::EndZone()
    {
#if defined(TRACY_ENABLE)
        if (m_context == nullptr)
            return;

        FE_Assert(!m_zones.empty());
        m_zones.pop_back();
#endif
    }


    void GpuProfiler::Collect([[maybe_unused]] const VkCommandBuffer commandBuffer)
    {
#if defined(TRACY_ENABLE)
        if (m_context == nullptr || m_usesHostQueryReset)
            return;

        FE_Assert(m_zones.empty());
        TracyVkCollect(m_context, commandBuffer);
#endif
    }


    void GpuProfiler::CollectIdle()
    {
#if defined(TRACY_ENABLE)
        if (m_context == nullptr || !m_hasUncollectedZones)
            return;

        FE_Assert(m_zones.empty());
        if (m_usesHostQueryReset)
        {
            for (uint32_t range = 0; range < 3; ++range)
                TracyVkCollect(m_context, VK_NULL_HANDLE);

            m_remainingHostQueries = m_context->GetQueryCount();
            m_hasUncollectedZones = false;
            return;
        }

        VerifyVk(vkGetFenceStatus(m_device, m_collectionFence));
        VerifyVk(vkResetCommandBuffer(m_collectionCommands, 0));
        const VkCommandBufferBeginInfo beginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                                  .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
        VerifyVk(vkBeginCommandBuffer(m_collectionCommands, &beginInfo));
        // Drain a previous partial collection and both sides of a wrapped query range.
        for (uint32_t range = 0; range < 3; ++range)
            TracyVkCollect(m_context, m_collectionCommands);

        VerifyVk(vkEndCommandBuffer(m_collectionCommands));
        VerifyVk(vkResetFences(m_device, 1, &m_collectionFence));
        const VkSubmitInfo submitInfo{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                       .commandBufferCount = 1,
                                       .pCommandBuffers = &m_collectionCommands };
        VerifyVk(vkQueueSubmit(m_queue, 1, &submitInfo, m_collectionFence));
        m_hasUncollectedZones = false;
#endif
    }
} // namespace FE::Graphics::Vulkan
