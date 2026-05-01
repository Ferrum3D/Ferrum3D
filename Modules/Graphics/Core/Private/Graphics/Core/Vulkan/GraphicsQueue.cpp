#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>

namespace FE::Graphics::Vulkan
{
    GraphicsQueue::GraphicsQueue(Core::Device* device)
    {
        FE_PROFILER_ZONE();

        m_device = device;

        m_fence = Fence::Create(m_device, 0);

        const VkCommandPool commandPool = ImplCast(device)->GetCommandPool(Core::DeviceQueueType::kGraphics);
        const uint32_t queueFamilyIndex = ImplCast(device)->GetQueueFamilyIndex(Core::DeviceQueueType::kGraphics);
        vkGetDeviceQueue(NativeCast(device), queueFamilyIndex, 0, &m_nativeQueue);

        for (uint32_t i = 0; i < kMaxInFlightFrames; ++i)
        {
            CommandBufferDesc desc;
            desc.m_name = Fmt::FormatName("GraphicsCommandBuffer_{}", i);
            desc.m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            desc.m_queue = m_nativeQueue;
            desc.m_commandPool = commandPool;
            desc.m_pageAllocator = &m_sharedPagePool;

            const Rc commandBuffer = CommandBuffer::Create(m_device, desc);
            commandBuffer->SetImmediateDestroyPolicy();
            m_graphicsCommandBuffers.push_back(commandBuffer);
        }
    }


    CommandBuffer* GraphicsQueue::GetCurrentCommandBuffer()
    {
        CommandBuffer* commandBuffer = m_graphicsCommandBuffers[m_frameIndex % kMaxInFlightFrames].Get();
        // FE_Assert(commandBuffer->IsRecording());
        return commandBuffer;
    }


    void GraphicsQueue::BeginFrame()
    {
        FE_PROFILER_ZONE();

        if (m_frameIndex > kMaxInFlightFrames)
            m_fence->Wait(m_frameIndex - kMaxInFlightFrames);

        GetCurrentCommandBuffer()->Begin();
        m_isActive = true;
    }


    Core::FenceSyncPoint GraphicsQueue::CloseFrame()
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_isActive);
        GetCurrentCommandBuffer()->Submit();
        return { m_fence, m_frameIndex++ };
    }


    void GraphicsQueue::Drain()
    {
        FE_PROFILER_ZONE();
        VerifyVk(vkQueueWaitIdle(m_nativeQueue));
    }
} // namespace FE::Graphics::Vulkan
