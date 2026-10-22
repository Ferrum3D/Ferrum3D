#include <Graphics/Core/Common/Buffer.h>
#include <Graphics/Core/Common/Texture.h>
#include <Graphics/Core/Vulkan/Barrier.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/GraphicsQueue.h>
#include <Graphics/Core/Vulkan/Viewport.h>

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

        m_gpuProfiler.Init(device, Core::DeviceQueueType::kGraphics, m_nativeQueue, "Vulkan Graphics");

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


    GraphicsQueue::~GraphicsQueue()
    {
        m_gpuProfiler.Shutdown();
    }


    CommandBuffer* GraphicsQueue::GetCurrentCommandBuffer()
    {
        CommandBuffer* commandBuffer = m_graphicsCommandBuffers[m_frameIndex % kMaxInFlightFrames].Get();
        if (!m_isRecording)
            BeginRecording();

        return commandBuffer;
    }


    void GraphicsQueue::BeginFrame()
    {
        FE_PROFILER_ZONE();

        FE_Assert(!m_isActive && !m_isRecording);
        m_trackedFenceValue = m_frameIndex.load();
        m_isActive = true;
        std::lock_guard guard(m_publicationLock);
        m_frameShaderReadBuffers.swap(m_pendingShaderReadBuffers);
        m_frameShaderReadTextures.swap(m_pendingShaderReadTextures);
    }


    void GraphicsQueue::BeginRecording()
    {
        FE_PROFILER_ZONE();
        FE_Assert(m_isActive && !m_isRecording);

        if (m_frameIndex > kMaxInFlightFrames)
            m_fence->Wait(m_frameIndex - kMaxInFlightFrames);

        CommandBuffer* commandBuffer = m_graphicsCommandBuffers[m_frameIndex % kMaxInFlightFrames].Get();
        commandBuffer->Begin();
        m_gpuProfiler.Collect(commandBuffer->GetNative());
        m_isRecording = true;

        for (const Rc<Core::Buffer>& resource : m_frameShaderReadBuffers)
        {
            auto* buffer = Common::ImplCast(resource.Get());
            const auto release = buffer->RetrieveQueueReleaseBarrier(Core::DeviceQueueType::kGraphics);
            FE_Assert(release.has_value(), "Published buffer has no transfer release");
            commandBuffer->EnqueueFenceToWait(release->m_completionFence);

            Core::BufferBarrierDesc barrier = release->m_barrier;
            barrier.m_syncBefore = Core::BarrierSyncFlags::kNone;
            barrier.m_accessBefore = Core::BarrierAccessFlags::kNone;
            barrier.m_syncAfter = Core::BarrierSyncFlags::kAllShading;
            barrier.m_accessAfter = Core::BarrierAccessFlags::kShaderRead;
            const VkBufferMemoryBarrier2 nativeBarrier = TranslateBarrier(barrier, ImplCast(m_device));
            VkDependencyInfo dependencyInfo = { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
            dependencyInfo.bufferMemoryBarrierCount = 1;
            dependencyInfo.pBufferMemoryBarriers = &nativeBarrier;
            vkCmdPipelineBarrier2(commandBuffer->GetNative(), &dependencyInfo);

            Common::SubresourceState state = buffer->GetState();
            state.m_sync = Core::BarrierSyncFlags::kAllShading;
            state.m_access = Core::BarrierAccessFlags::kShaderRead;
            state.m_queueType = Core::DeviceQueueType::kGraphics;
            buffer->SetState(state);
        }
        m_frameShaderReadBuffers.clear();

        for (const Rc<Core::Texture>& resource : m_frameShaderReadTextures)
        {
            auto* texture = Common::ImplCast(resource.Get());
            const Core::TextureDesc desc = texture->GetDesc();
            const Core::TextureSubresourceIterator subresources{ Core::TextureSubresource::CreateWhole(desc) };
            for (const auto [mipIndex, arrayIndex] : subresources)
            {
                const Core::TextureSubresource subresource = Core::TextureSubresource::Create(desc, mipIndex, arrayIndex);
                const auto release = texture->RetrieveQueueReleaseBarrier(Core::DeviceQueueType::kGraphics, subresource);
                FE_Assert(release.has_value(), "Published texture subresource has no transfer release");
                commandBuffer->EnqueueFenceToWait(release->m_completionFence);

                Core::TextureBarrierDesc barrier = release->m_barrier;
                barrier.m_syncBefore = Core::BarrierSyncFlags::kNone;
                barrier.m_accessBefore = Core::BarrierAccessFlags::kNone;
                barrier.m_syncAfter = Core::BarrierSyncFlags::kAllShading;
                barrier.m_accessAfter = Core::BarrierAccessFlags::kShaderRead;
                const VkImageMemoryBarrier2 nativeBarrier = TranslateBarrier(barrier, ImplCast(m_device));
                VkDependencyInfo dependencyInfo = { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
                dependencyInfo.imageMemoryBarrierCount = 1;
                dependencyInfo.pImageMemoryBarriers = &nativeBarrier;
                vkCmdPipelineBarrier2(commandBuffer->GetNative(), &dependencyInfo);

                Common::SubresourceState state = texture->GetState(subresource);
                state.m_sync = Core::BarrierSyncFlags::kAllShading;
                state.m_access = Core::BarrierAccessFlags::kShaderRead;
                state.m_layout = Core::BarrierLayout::kShaderRead;
                state.m_queueType = Core::DeviceQueueType::kGraphics;
                texture->SetState(subresource, state);
            }
        }
        m_frameShaderReadTextures.clear();
    }


    Core::FenceSyncPoint GraphicsQueue::GetCurrentFence() const
    {
        FE_Assert(m_isActive);
        return { .m_fence = m_fence, .m_value = m_frameIndex };
    }


    void GraphicsQueue::PublishShaderRead(Core::Buffer* buffer)
    {
        FE_Assert(buffer);
        std::lock_guard guard(m_publicationLock);
        m_pendingShaderReadBuffers.push_back(buffer);
    }


    void GraphicsQueue::PublishShaderRead(Core::Texture* texture)
    {
        FE_Assert(texture);
        std::lock_guard guard(m_publicationLock);
        m_pendingShaderReadTextures.push_back(texture);
    }


    Core::FenceSyncPoint GraphicsQueue::CloseFrame()
    {
        FE_Assert(m_isActive);
        const Core::FenceSyncPoint syncPoint{ .m_fence = m_fence, .m_value = m_frameIndex };
        ++m_frameIndex;
        m_isActive = false;
        m_isRecording = false;
        return syncPoint;
    }


    void GraphicsQueue::SubmitFrame(const festd::span<Core::Viewport* const> presentedViewports)
    {
        FE_PROFILER_ZONE();

        CommandBuffer* commandBuffer = GetCurrentCommandBuffer();
        for (Core::Viewport* viewport : presentedViewports)
            ImplCast(viewport)->RecordPresent(commandBuffer);

        const Core::FenceSyncPoint completion = CloseFrame();
        commandBuffer->EnqueueFenceToSignal(completion);
        commandBuffer->Submit();

        for (Core::Viewport* viewport : presentedViewports)
            ImplCast(viewport)->QueuePresent(completion);
    }


    void GraphicsQueue::Drain()
    {
        FE_PROFILER_ZONE();
        VerifyVk(vkQueueWaitIdle(m_nativeQueue));
    }
} // namespace FE::Graphics::Vulkan
