#include <Graphics/Core/Vulkan/AsyncCopyQueue.h>
#include <Graphics/Core/Vulkan/Device.h>
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
    } // namespace


    void AsyncCopyQueue::ThreadProc()
    {
        for (;;)
        {
            m_threadEvent.Wait();
            m_threadRunning = true;

            FE_PROFILER_ZONE_NAMED("AsyncCopyThread");

            FinalizeFinishedProcessors();

            bool anyProcessing = false;
            uint32_t freeProcessorIndex = kInvalidIndex;
            for (uint32_t i = 0; i < kMaxInFlightSubmits; ++i)
            {
                const auto& processor = m_inFlightProcessors[i];
                if (processor.m_isProcessing || processor.m_isQueued)
                    anyProcessing = true;
                else
                    freeProcessorIndex = i;
            }

            if (freeProcessorIndex != kInvalidIndex)
            {
                uint32_t slotToProcess;
                {
                    std::lock_guard lock{ m_lock };
                    slotToProcess = m_queuedSlots.find_first();
                    if (slotToProcess != kInvalidIndex)
                    {
                        m_queuedSlots.reset(slotToProcess);
                        m_inFlightProcessors[freeProcessorIndex].m_slotIndex = slotToProcess;
                        m_inFlightProcessors[freeProcessorIndex].m_isQueued = true;
                    }
                }

                if (slotToProcess != kInvalidIndex)
                {
                    anyProcessing = true;
                    ProcessCommandList(freeProcessorIndex);
                }
            }

            if (!anyProcessing)
            {
                std::lock_guard lock{ m_lock };
                if (m_queuedSlots.find_first() == kInvalidIndex)
                {
                    m_threadEvent.Reset();
                    m_threadRunning = false;
                }
            }
        }
    }


    bool AsyncCopyQueue::FinalizeFinishedProcessors()
    {
        FE_PROFILER_ZONE();

        bool anyFinalized = false;
        for (uint32_t i = 0; i < kMaxInFlightSubmits; ++i)
        {
            auto& processor = m_inFlightProcessors[i];
            if (processor.m_isProcessing)
            {
                const uint32_t slotIndex = processor.m_slotIndex;
                Slot& slot = m_slots[slotIndex];
                const VkResult fenceStatus = vkGetFenceStatus(NativeCast(m_device), processor.m_fence);
                if (fenceStatus == VK_SUCCESS)
                {
                    vmaVirtualFree(m_uploadBufferBlock, processor.m_stagingAllocation);

                    processor.m_slotIndex = kInvalidIndex;
                    processor.m_isProcessing = false;
                    processor.m_isQueued = false;

                    std::lock_guard lock{ m_lock };

                    slot.m_waitGroup->Signal();
                    slot.m_waitGroup.Reset();

                    m_freeSlots.set(slotIndex);

                    anyFinalized = true;
                }
                else
                {
                    FE_Assert(fenceStatus == VK_NOT_READY);
                }
            }
        }

        return anyFinalized;
    }


    void AsyncCopyQueue::ProcessCommandList(const uint32_t processorIndex)
    {
        FE_PROFILER_ZONE();

        InFlightProcessor& processor = m_inFlightProcessors[processorIndex];
        if (processor.m_commandBuffer)
        {
            VerifyVulkan(vkResetCommandBuffer(processor.m_commandBuffer->GetNative(), 0));
            VerifyVulkan(vkResetFences(NativeCast(m_device), 1, &processor.m_fence));
        }
        else
        {
            FE_Assert(processor.m_fence == VK_NULL_HANDLE);

            const Env::Name name = Fmt::FormatName("AsyncCopyCommandBuffer_{}", processorIndex);
            processor.m_commandBuffer = CommandBuffer::Create(m_device, name, Core::HardwareQueueKindFlags::kTransfer);

            VkFenceCreateInfo fenceCI = {};
            fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            vkCreateFence(NativeCast(m_device), &fenceCI, nullptr, &processor.m_fence);
        }

        const Slot& slot = m_slots[processor.m_slotIndex];

        const VkCommandBuffer commandBuffer = processor.m_commandBuffer->GetNative();

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VerifyVulkan(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        CommandBatcher batcher;
        batcher.m_commandBuffer = commandBuffer;

        Memory::SegmentedBufferReader reader{ slot.m_commandList.m_buffer };
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

                    VmaVirtualAllocationCreateInfo stagingAllocationCI = {};
                    stagingAllocationCI.size = cmd.m_size;
                    stagingAllocationCI.alignment = kStagingAllocationAlignment;

                    FE_Assert(cmd.m_size <= kUploadBufferSize, "Not supported yet.");

                    VkDeviceSize allocationOffset;
                    for (;;)
                    {
                        const VkResult allocationResult = vmaVirtualAllocate(
                            m_uploadBufferBlock, &stagingAllocationCI, &processor.m_stagingAllocation, &allocationOffset);

                        if (allocationResult == VK_SUCCESS)
                            break;

                        FE_PROFILER_ZONE_NAMED("Stall");

                        while (!FinalizeFinishedProcessors())
                        {
                            for (uint32_t i = 0; i < 32; ++i)
                                _mm_pause();
                        }
                    }

                    void* data = static_cast<std::byte*>(m_uploadBuffer->Map()) + allocationOffset;
                    memcpy(data, cmd.m_data, cmd.m_size);
                    m_uploadBuffer->Unmap();

                    VkBufferCopy copy;
                    copy.srcOffset = allocationOffset;
                    copy.dstOffset = cmd.m_destinationOffset;
                    copy.size = cmd.m_size;
                    vkCmdCopyBuffer(commandBuffer, m_uploadBuffer->GetNative(), NativeCast(cmd.m_buffer), 1, &copy);
                    break;
                }

            case AsyncCopyCommandType::kInvalid:
            default:
                FE_DebugBreak();
                break;
            }
        }

        batcher.Flush();

        VerifyVulkan(vkEndCommandBuffer(commandBuffer));

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VerifyVulkan(vkQueueSubmit(m_queue, 1, &submitInfo, processor.m_fence));

        processor.m_isProcessing = true;
    }


    AsyncCopyQueue::AsyncCopyQueue(Core::Device* device, Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
        m_device = device;

        m_thread = Threading::CreateThread(
            "AsyncCopyThread",
            [](const uintptr_t data) {
                reinterpret_cast<AsyncCopyQueue*>(data)->ThreadProc();
            },
            reinterpret_cast<uintptr_t>(this));

        m_threadEvent = Threading::Event::CreateManualReset();

        const Core::BufferDesc uploadDesc{ kUploadBufferSize, Core::BindFlags::kNone, Core::ResourceUsage::kHostWriteThrough };
        m_uploadBuffer = ImplCast(m_resourcePool->CreateBuffer("AsyncUploadBuffer", uploadDesc));

        VmaVirtualBlockCreateInfo virtualBlockCI = {};
        virtualBlockCI.size = kUploadBufferSize;
        VerifyVulkan(vmaCreateVirtualBlock(&virtualBlockCI, &m_uploadBufferBlock));

        const auto* vkDevice = ImplCast(m_device);
        const uint32_t queueFamilyIndex = vkDevice->GetQueueFamilyIndex(Core::HardwareQueueKindFlags::kTransfer);
        vkGetDeviceQueue(vkDevice->GetNative(), queueFamilyIndex, 0, &m_queue);
    }


    AsyncCopyQueue::~AsyncCopyQueue()
    {
        Drain();
        Threading::CloseThread(m_thread);
    }


    Rc<WaitGroup> AsyncCopyQueue::ExecuteCommandList(const Core::AsyncCopyCommandList& commandList)
    {
        std::lock_guard lock{ m_lock };

        uint32_t slotIndex = m_freeSlots.find_first();
        if (slotIndex == kInvalidIndex)
        {
            m_freeSlots.resize(Math::Max(m_freeSlots.size() * 2, 256u), true);
            m_queuedSlots.resize(Math::Max(m_queuedSlots.size() * 2, 256u), false);
            m_slots.resize(Math::Max(m_slots.size() * 2, 256u));
            slotIndex = m_freeSlots.find_first();
        }

        FE_Assert(slotIndex != kInvalidIndex);

        m_freeSlots.reset(slotIndex);
        m_queuedSlots.set(slotIndex);

        Slot& slot = m_slots[slotIndex];
        slot.m_waitGroup = WaitGroup::Create();
        slot.m_commandList = commandList;
        m_threadEvent.Send();
        return slot.m_waitGroup;
    }


    void AsyncCopyQueue::Drain()
    {
        while (m_threadRunning > 0)
        {
            // TODO: wait
            for (uint32_t i = 0; i < 32; ++i)
                _mm_pause();
        }

        VerifyVulkan(vkQueueWaitIdle(m_queue));
    }
} // namespace FE::Graphics::Vulkan
