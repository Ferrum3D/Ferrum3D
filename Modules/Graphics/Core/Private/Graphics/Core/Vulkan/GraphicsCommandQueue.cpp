#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/GraphicsCommandQueue.h>

namespace FE::Graphics::Vulkan
{
    GraphicsCommandQueue::GraphicsCommandQueue(Core::Device* device)
    {
        m_device = device;

        m_fence = Fence::Create(m_device);
        m_fence->Init(0);

        const VkCommandPool commandPool = ImplCast(device)->GetCommandPool(Core::HardwareQueueKindFlags::kGraphics);
        const uint32_t queueFamilyIndex = ImplCast(device)->GetQueueFamilyIndex(Core::HardwareQueueKindFlags::kGraphics);
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


    CommandBuffer* GraphicsCommandQueue::GetCurrentGraphicsCommandBuffer()
    {
        return m_graphicsCommandBuffers[m_frameIndex % kMaxInFlightFrames].Get();
    }


    void GraphicsCommandQueue::WaitForPreviousFrame()
    {
        if (m_frameIndex > kMaxInFlightFrames)
        {
            m_fence->Wait(m_frameIndex - kMaxInFlightFrames);
        }
    }


    Core::FenceSyncPoint GraphicsCommandQueue::CloseFrame()
    {
        return { m_fence, m_frameIndex++ };
    }
} // namespace FE::Graphics::Vulkan
