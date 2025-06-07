#include <Graphics/Core/Vulkan/AsyncCopyQueue.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Fence.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        struct CommandBatcher final
        {
            VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
            festd::small_vector<VkBufferCopy> m_bufferRegions;
            VkBuffer m_srcBuffer = VK_NULL_HANDLE;
            VkBuffer m_dstBuffer = VK_NULL_HANDLE;

            void Flush()
            {
                if (!m_bufferRegions.empty())
                {
                    vkCmdCopyBuffer(m_commandBuffer, m_srcBuffer, m_dstBuffer, m_bufferRegions.size(), m_bufferRegions.data());
                    m_bufferRegions.clear();
                }
            }
        };


        struct ScopedMapper final
        {
            Buffer* m_buffer = nullptr;
            void* m_data = nullptr;

            std::byte* Map()
            {
                if (m_data == nullptr)
                    m_data = m_buffer->Map();

                return static_cast<std::byte*>(m_data);
            }

            ~ScopedMapper()
            {
                if (m_data != nullptr)
                    m_buffer->Unmap();
            }
        };
    } // namespace


    void AsyncCopyQueue::ThreadProc()
    {
        for (;;)
        {
            m_threadEvent.Wait();

            if (m_exitRequested)
                break;

            FE_PROFILER_ZONE_NAMED("AsyncCopyThread");

            FinalizeFinishedProcessors();

            if (m_requestCache.empty())
            {
                auto* item = static_cast<Core::AsyncCopyCommandList*>(m_requestQueue.DequeueAll());
                while (item)
                {
                    m_requestCache.push_back(item);
                    item = static_cast<Core::AsyncCopyCommandList*>(item->m_next);
                }
            }

            Core::AsyncCopyCommandList* item = nullptr;
            if (!m_requestCache.empty())
            {
                item = m_requestCache.back();
                m_requestCache.pop_back();
            }

            if (item)
            {
                ProcessingItem* processingItem = m_processingItemPool.New();
                processingItem->m_queueItem = *item;
                processingItem->m_fenceValue = ++m_fenceValue;

                if (m_processingItems.full())
                    m_processingItems.reserve(m_processingItems.capacity() * 2);

                m_processingItems.push_back(processingItem);

                ProcessCommandList(processingItem);
            }
            else if (m_requestCache.empty() && m_processingItems.empty())
            {
                // Block producers to ensure no one adds new requests before we go idle.
                std::unique_lock lock{ m_suspendLock };
                if (m_requestQueue.Empty())
                {
                    m_threadEvent.Reset();
                    m_suspendEvent.Send();
                }
            }
        }
    }


    bool AsyncCopyQueue::FinalizeFinishedProcessors(const bool wait)
    {
        FE_PROFILER_ZONE();

        bool loopedOnce = false;
        for (;;)
        {
            bool anyProgress = false;
            while (!m_processingItems.empty())
            {
                ProcessingItem* item = m_processingItems.front();
                if (item->m_fenceValue > m_fence->GetCompletedValue())
                    break;

                FE_Assert(item->m_fenceValue != 0);

                if (item->m_queueItem.m_signalWaitGroup)
                    item->m_queueItem.m_signalWaitGroup->Signal();

                m_freeCommandBuffers.push_back(item->m_commandBuffer);
                for (const VmaVirtualAllocation stagingAllocation : item->m_stagingAllocations)
                    vmaVirtualFree(m_uploadRingBuffer, stagingAllocation);

                m_processingItems.pop_front();
                m_processingItemPool.Delete(item);

                anyProgress = true;
            }

            if (!anyProgress && wait)
            {
                if (m_processingItems.empty())
                    return false;

                FE_Assert(!loopedOnce, "Infinite loop detected");
                m_fence->Wait(m_processingItems.front()->m_fenceValue);
                loopedOnce = true;
                continue;
            }

            return true;
        }
    }


    void AsyncCopyQueue::ProcessCommandList(ProcessingItem* item)
    {
        FE_PROFILER_ZONE();

        FE_Assert(item->m_commandBuffer == nullptr);

        item->m_commandBuffer = AcquireCommandBuffer();

        const VkCommandBuffer commandBuffer = item->m_commandBuffer->GetNative();

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VerifyVulkan(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        CommandBatcher batcher;
        batcher.m_commandBuffer = commandBuffer;

        ScopedMapper mapper;
        mapper.m_buffer = m_uploadBuffer.Get();

        Memory::SegmentedBufferReader reader{ item->m_queueItem.m_buffer };
        for (;;)
        {
            using namespace Core::InternalAsyncCopyCommands;

            AsyncCopyCommandType commandType;
            if (!reader.ReadNoConsume(commandType))
                break;

            switch (commandType)
            {
            case AsyncCopyCommandType::kCopyBuffer:
                {
                    batcher.Flush();

                    AsyncCopyBufferCommand cmd;
                    FE_Verify(reader.Read(cmd));

                    VkBufferCopy copy;
                    copy.srcOffset = cmd.m_sourceOffset;
                    copy.dstOffset = cmd.m_destinationOffset;
                    copy.size = cmd.m_size;
                    batcher.m_bufferRegions.push_back(copy);
                    batcher.m_srcBuffer = NativeCast(cmd.m_source);
                    batcher.m_dstBuffer = NativeCast(cmd.m_destination);
                    break;
                }

            case AsyncCopyCommandType::kCopyBufferContinuation:
                {
                    AsyncCopyBufferContinuationCommand cmd;
                    FE_Verify(reader.Read(cmd));

                    VkBufferCopy copy;
                    copy.srcOffset = cmd.m_sourceOffset;
                    copy.dstOffset = cmd.m_destinationOffset;
                    copy.size = cmd.m_size;
                    batcher.m_bufferRegions.push_back(copy);
                    break;
                }

            case AsyncCopyCommandType::kUploadBuffer:
                {
                    batcher.Flush();

                    AsyncUploadBufferCommand cmd;
                    FE_Verify(reader.Read(cmd));

                    uint32_t uploadedBytes = 0;
                    while (uploadedBytes < cmd.m_size)
                    {
                        const uint32_t allocationSize = Math::Min(cmd.m_size - uploadedBytes, kUploadBufferSize);

                        const VkDeviceSize allocationOffset =
                            AllocateStagingMemory(item, allocationSize, kStagingAllocationAlignment);

                        auto* data = mapper.Map();
                        auto* copyDestination = data + allocationOffset;
                        const auto* copySource = static_cast<const std::byte*>(cmd.m_data) + uploadedBytes;
                        memcpy(copyDestination, copySource, allocationSize);

                        VkBufferCopy copy;
                        copy.srcOffset = allocationOffset;
                        copy.dstOffset = cmd.m_destinationOffset;
                        copy.size = cmd.m_size;
                        vkCmdCopyBuffer(commandBuffer, m_uploadBuffer->GetNative(), NativeCast(cmd.m_buffer), 1, &copy);

                        uploadedBytes += allocationSize;
                    }

                    break;
                }

            case AsyncCopyCommandType::kInvalid:
            default:
                FE_DebugBreak();
                break;
            }
        }

        batcher.Flush();
        SubmitCommandList(commandBuffer, item->m_fenceValue);
    }


    VkDeviceSize AsyncCopyQueue::AllocateStagingMemory(ProcessingItem* item, const size_t byteSize, const size_t byteAlignment)
    {
        VmaVirtualAllocationCreateInfo stagingAllocationCI = {};
        stagingAllocationCI.size = byteSize;
        stagingAllocationCI.alignment = byteAlignment;

        const VkCommandBuffer commandBuffer = item->m_commandBuffer->GetNative();

        item->m_stagingAllocations.push_back(VK_NULL_HANDLE);

        VkDeviceSize allocationOffset;
        for (;;)
        {
            const VkResult allocationResult = vmaVirtualAllocate(
                m_uploadRingBuffer, &stagingAllocationCI, &item->m_stagingAllocations.back(), &allocationOffset);

            if (allocationResult == VK_SUCCESS)
                break;

            FE_PROFILER_ZONE_NAMED("Stall");
            if (!FinalizeFinishedProcessors(true))
            {
                //
                // We could not free any staging memory by waiting for already submitted commands to finish.
                // We can try to submit the current command buffer and wait for it to finish.
                //

                if (item->m_stagingAllocations.empty())
                {
                    // We did not allocate any staging memory for the current submit, there is nothing we can do.
                    FE_DebugBreak();
                    break;
                }

                SubmitCommandList(commandBuffer, item->m_fenceValue);
                m_fence->Wait(item->m_fenceValue);

                for (const VmaVirtualAllocation stagingAllocation : item->m_stagingAllocations)
                    vmaVirtualFree(m_uploadRingBuffer, stagingAllocation);

                item->m_stagingAllocations.clear();
                item->m_stagingAllocations.push_back(VK_NULL_HANDLE);

                item->m_fenceValue = ++m_fenceValue;

                // We can use the same command buffer since we have waited for it to finish.

                VkCommandBufferBeginInfo beginInfo = {};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                VerifyVulkan(vkBeginCommandBuffer(commandBuffer, &beginInfo));
            }
        }

        return allocationOffset;
    }


    void AsyncCopyQueue::SubmitCommandList(const VkCommandBuffer commandBuffer, const uint64_t fenceValue) const
    {
        VerifyVulkan(vkEndCommandBuffer(commandBuffer));

        VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo = {};
        timelineSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        timelineSemaphoreSubmitInfo.pSignalSemaphoreValues = &fenceValue;
        timelineSemaphoreSubmitInfo.signalSemaphoreValueCount = 1;

        const VkSemaphore signalSemaphore = NativeCast(m_fence.Get());

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = &timelineSemaphoreSubmitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        VerifyVulkan(vkQueueSubmit(m_queue, 1, &submitInfo, nullptr));
    }


    Rc<CommandBuffer> AsyncCopyQueue::AcquireCommandBuffer()
    {
        if (!m_freeCommandBuffers.empty())
        {
            const auto commandBuffer = m_freeCommandBuffers.back();
            m_freeCommandBuffers.pop_back();
            return commandBuffer;
        }

        return CommandBuffer::Create(m_device,
                                     Fmt::FormatName("AsyncCopyCommandBuffer_{}", m_commandBufferCounter++),
                                     Core::HardwareQueueKindFlags::kTransfer);
    }


    AsyncCopyQueue::AsyncCopyQueue(Core::Device* device, Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
        m_device = device;

        m_processingItems.reserve(m_processingItems.get_container().capacity());

        m_thread = Threading::CreateThread(
            "AsyncCopyThread",
            [](const uintptr_t data) {
                reinterpret_cast<AsyncCopyQueue*>(data)->ThreadProc();
            },
            reinterpret_cast<uintptr_t>(this));

        m_threadEvent = Threading::Event::CreateManualReset();
        m_suspendEvent = Threading::Event::CreateManualReset();

        const Core::BufferDesc uploadDesc{ kUploadBufferSize, Core::BindFlags::kNone, Core::ResourceUsage::kHostWriteThrough };
        m_uploadBuffer = ImplCast(m_resourcePool->CreateBuffer("AsyncUploadBuffer", uploadDesc));

        m_fence = Fence::Create(m_device);
        m_fence->Init();

        VmaVirtualBlockCreateInfo virtualBlockCI = {};
        virtualBlockCI.size = kUploadBufferSize;
        virtualBlockCI.flags = VMA_VIRTUAL_BLOCK_CREATE_LINEAR_ALGORITHM_BIT;
        VerifyVulkan(vmaCreateVirtualBlock(&virtualBlockCI, &m_uploadRingBuffer));

        const auto* vkDevice = ImplCast(m_device);
        const uint32_t queueFamilyIndex = vkDevice->GetQueueFamilyIndex(Core::HardwareQueueKindFlags::kTransfer);
        vkGetDeviceQueue(vkDevice->GetNative(), queueFamilyIndex, 0, &m_queue);
    }


    AsyncCopyQueue::~AsyncCopyQueue()
    {
        {
            std::unique_lock lock{ m_suspendLock };
            m_exitRequested = true;
        }

        Threading::CloseThread(m_thread);
    }


    void AsyncCopyQueue::ExecuteCommandList(Core::AsyncCopyCommandList* commandList)
    {
        std::shared_lock lock{ m_suspendLock };

        m_requestQueue.Enqueue(commandList);
        m_threadEvent.Send();
    }


    void AsyncCopyQueue::Drain()
    {
        {
            std::unique_lock lock{ m_suspendLock };
            m_suspendEvent.Reset();
            m_threadEvent.Send();
        }

        m_suspendEvent.Wait();
        FE_Assert(m_requestQueue.Empty());
        FE_Assert(m_requestCache.empty());
        VerifyVulkan(vkQueueWaitIdle(m_queue));
    }
} // namespace FE::Graphics::Vulkan
