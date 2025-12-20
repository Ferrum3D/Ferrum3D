#include <Graphics/Core/Common/ResourceBarrierBatcher.h>
#include <Graphics/Core/Vulkan/AsyncCopyQueue.h>
#include <Graphics/Core/Vulkan/Barrier.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Fence.h>
#include <Graphics/Core/Vulkan/Format.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>
#include <Graphics/Core/Vulkan/Texture.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        void FlushResourceBarriers(std::pmr::memory_resource* allocator, const Device* device,
                                   const VkCommandBuffer commandBuffer, const Common::ResourceBarrierBatcher& batcher)
        {
            festd::pmr::vector<VkImageMemoryBarrier2> imageBarriers{ allocator };
            festd::pmr::vector<VkBufferMemoryBarrier2> bufferBarriers{ allocator };

            for (const Core::TextureBarrierDesc& barrier : batcher.m_textureBarriers)
                imageBarriers.push_back(TranslateBarrier(barrier, device));

            for (const Core::BufferBarrierDesc& barrier : batcher.m_bufferBarriers)
                bufferBarriers.push_back(TranslateBarrier(barrier, device));

            VkDependencyInfo dependencyInfo = {};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependencyInfo.imageMemoryBarrierCount = imageBarriers.size();
            dependencyInfo.pImageMemoryBarriers = imageBarriers.data();
            dependencyInfo.bufferMemoryBarrierCount = bufferBarriers.size();
            dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();
            vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
        }


        struct CommandBatcher final
        {
            VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
            festd::pmr::vector<VkBufferCopy> m_bufferRegions;
            VkBuffer m_srcBuffer = VK_NULL_HANDLE;
            VkBuffer m_dstBuffer = VK_NULL_HANDLE;

            explicit CommandBatcher(std::pmr::memory_resource* allocator, const VkCommandBuffer commandBuffer)
                : m_commandBuffer(commandBuffer)
                , m_bufferRegions(allocator)
            {
                m_bufferRegions.reserve(256);
            }

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

                if (item->m_allocator)
                    Memory::Delete(item->m_allocator, item);

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

        Memory::LinearAllocator::Scope tempAllocatorScope{ m_threadTempAllocator };

        item->m_commandBuffer = AcquireCommandBuffer();
        item->m_commandBuffer->Begin();

        const VkCommandBuffer commandBuffer = item->m_commandBuffer->GetNative();

        CommandBatcher batcher{ &m_threadTempAllocator, commandBuffer };

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
            case AsyncCopyCommandType::kInvokeFunctor:
                {
                    FE_PROFILER_ZONE_NAMED("AsyncCopyQueue::InvokeFunctor");

                    AsyncInvokeFunctorCommand cmd;
                    FE_Verify(reader.Read(cmd));

                    cmd.m_functor(cmd.m_context);
                    FE_Verify(reader.SkipBytes(cmd.m_functorSize));
                    break;
                }

            case AsyncCopyCommandType::kCopyBuffer:
                {
                    FE_PROFILER_ZONE_NAMED("AsyncCopyQueue::CopyBuffer");

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
                    FE_PROFILER_ZONE_NAMED("AsyncCopyQueue::CopyBufferContinuation");

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
                    FE_PROFILER_ZONE_NAMED("AsyncCopyQueue::UploadBuffer");

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

            case AsyncCopyCommandType::kUploadTexture:
                {
                    FE_PROFILER_ZONE_NAMED("AsyncCopyQueue::UploadTexture");

                    batcher.Flush();

                    AsyncUploadTextureCommand cmd;
                    FE_Verify(reader.Read(cmd));

                    auto* texture = const_cast<Texture*>(ImplCast(cmd.m_texture));
                    const Core::TextureDesc imageDesc = texture->GetDesc();
                    const Core::FormatInfo formatInfo{ imageDesc.m_imageFormat };
                    const Core::TextureSubresource subresource = cmd.m_subresource;

                    FE_Assert(subresource.m_firstArraySlice + subresource.m_arraySize <= imageDesc.m_arraySize);
                    FE_Assert(subresource.m_mostDetailedMipSlice + subresource.m_mipSliceCount <= imageDesc.m_mipSliceCount);

                    festd::pmr::vector<VkBufferImageCopy> bufferImageCopies{ &m_threadTempAllocator };
                    bufferImageCopies.reserve(subresource.m_arraySize * subresource.m_mipSliceCount);

                    Common::ResourceBarrierBatcher beforeBarrierBatcher{ &m_threadTempAllocator };
                    Common::ResourceBarrierBatcher afterBarrierBatcher{ &m_threadTempAllocator };

                    const Core::TextureSubresourceIterator subresourceIterator{ subresource };

                    uint32_t uploadedBytes = 0;
                    for (const auto [mipIndex, arrayIndex] : subresourceIterator)
                    {
                        const uint32_t allocationSize = formatInfo.CalculateMipByteSize(imageDesc.GetSize(), mipIndex);

                        FE_Assert(allocationSize <= kUploadBufferSize,
                                  "Currently, each mip level must entirely fit into staging buffer");

                        const VkDeviceSize allocationOffset =
                            AllocateStagingMemory(item, allocationSize, kStagingAllocationAlignment);

                        auto* data = mapper.Map();
                        auto* copyDestination = data + allocationOffset;
                        const auto* copySource = static_cast<const std::byte*>(cmd.m_data) + uploadedBytes;

                        Memory::AssertPointerIsValid(cmd.m_data);

                        memcpy(copyDestination, copySource, allocationSize);

                        const auto currentSubresource = Core::TextureSubresource::Create(imageDesc, mipIndex, arrayIndex);
                        Common::SubresourceState subresourceState = texture->GetState(currentSubresource);

                        // Resource must be discarded before being used as transfer destination.
                        FE_Assert(subresourceState.m_layout == Core::BarrierLayout::kUndefined);

                        // Transition the image to transfer destination
                        Core::TextureBarrierDesc barrierDesc;
                        barrierDesc.m_syncBefore = subresourceState.m_sync;
                        barrierDesc.m_syncAfter = Core::BarrierSyncFlags::kCopy;
                        barrierDesc.m_accessBefore = subresourceState.m_access;
                        barrierDesc.m_accessAfter = Core::BarrierAccessFlags::kCopyDest;
                        barrierDesc.m_layoutBefore = subresourceState.m_layout;
                        barrierDesc.m_layoutAfter = Core::BarrierLayout::kCopyDest;
                        barrierDesc.m_subresource = currentSubresource;
                        barrierDesc.m_queueBefore = Core::DeviceQueueType::kCount;
                        barrierDesc.m_queueAfter = Core::DeviceQueueType::kCount;
                        beforeBarrierBatcher.AddBarrier(barrierDesc);

                        subresourceState.m_sync = Core::BarrierSyncFlags::kCopy;
                        subresourceState.m_access = Core::BarrierAccessFlags::kCopyDest;
                        subresourceState.m_layout = Core::BarrierLayout::kCopyDest;
                        subresourceState.m_queueType = Core::DeviceQueueType::kTransfer;
                        texture->SetState(currentSubresource, subresourceState);

                        // Release the image from the transfer queue
                        barrierDesc.m_syncBefore = Core::BarrierSyncFlags::kCopy;
                        barrierDesc.m_accessBefore = Core::BarrierAccessFlags::kCopyDest;
                        barrierDesc.m_layoutBefore = Core::BarrierLayout::kCopyDest;
                        barrierDesc.m_queueAfter = Core::DeviceQueueType::kGraphics;
                        afterBarrierBatcher.AddBarrier(barrierDesc);

                        VkBufferImageCopy copy = {};
                        copy.imageSubresource.aspectMask = TranslateImageAspectFlags(imageDesc.m_imageFormat);
                        copy.imageSubresource.mipLevel = mipIndex;
                        copy.imageSubresource.baseArrayLayer = arrayIndex;
                        copy.imageSubresource.layerCount = 1;
                        copy.imageExtent.width = Math::Max(1u, imageDesc.m_width >> mipIndex);
                        copy.imageExtent.height = Math::Max(1u, imageDesc.m_height >> mipIndex);
                        copy.imageExtent.depth = Math::Max(1u, imageDesc.m_depth >> mipIndex);
                        copy.imageOffset.x = 0;
                        copy.imageOffset.y = 0;
                        copy.imageOffset.z = 0;
                        copy.bufferOffset = allocationOffset;
                        bufferImageCopies.push_back(copy);

                        uploadedBytes += allocationSize;
                    }

                    FlushResourceBarriers(&m_threadTempAllocator, ImplCast(m_device), commandBuffer, beforeBarrierBatcher);

                    vkCmdCopyBufferToImage(commandBuffer,
                                           m_uploadBuffer->GetNative(),
                                           texture->GetNative(),
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           bufferImageCopies.size(),
                                           bufferImageCopies.data());

                    FlushResourceBarriers(&m_threadTempAllocator, ImplCast(m_device), commandBuffer, afterBarrierBatcher);

                    break;
                }

            case AsyncCopyCommandType::kInvalid:
            default:
                FE_DebugBreak();
                break;
            }
        }

        batcher.Flush();
        item->m_commandBuffer->EnqueueFenceToSignal({ m_fence, item->m_fenceValue });
        item->m_commandBuffer->Submit();
        item->m_queueItem.m_buffer.Free();
    }


    VkDeviceSize AsyncCopyQueue::AllocateStagingMemory(ProcessingItem* item, const size_t byteSize, const size_t byteAlignment)
    {
        VmaVirtualAllocationCreateInfo stagingAllocationCI = {};
        stagingAllocationCI.size = byteSize;
        stagingAllocationCI.alignment = byteAlignment;

        item->m_stagingAllocations.push_back(VK_NULL_HANDLE);

        VkDeviceSize allocationOffset;
        for (;;)
        {
            const VkResult allocationResult = vmaVirtualAllocate(m_uploadRingBuffer,
                                                                 &stagingAllocationCI,
                                                                 &item->m_stagingAllocations.back(),
                                                                 &allocationOffset);

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

                item->m_commandBuffer->EnqueueFenceToSignal({ m_fence, item->m_fenceValue });
                item->m_commandBuffer->Submit();
                m_fence->Wait(item->m_fenceValue);

                for (const VmaVirtualAllocation stagingAllocation : item->m_stagingAllocations)
                    vmaVirtualFree(m_uploadRingBuffer, stagingAllocation);

                item->m_stagingAllocations.clear();
                item->m_stagingAllocations.push_back(VK_NULL_HANDLE);
                item->m_fenceValue = ++m_fenceValue;

                // We can use the same command buffer since we have waited for it to finish.
                item->m_commandBuffer->Begin();
            }
        }

        return allocationOffset;
    }


    Rc<CommandBuffer> AsyncCopyQueue::AcquireCommandBuffer()
    {
        if (!m_freeCommandBuffers.empty())
        {
            const auto commandBuffer = m_freeCommandBuffers.back();
            m_freeCommandBuffers.pop_back();
            return commandBuffer;
        }

        CommandBufferDesc desc;
        desc.m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        desc.m_commandPool = ImplCast(m_device)->GetCommandPool(Core::DeviceQueueType::kTransfer);
        desc.m_name = Fmt::FormatName("AsyncCopyCommandBuffer_{}", m_commandBufferCounter++);
        desc.m_pageAllocator = std::pmr::get_default_resource();
        desc.m_queue = m_queue;

        return CommandBuffer::Create(m_device, desc);
    }


    AsyncCopyQueue::AsyncCopyQueue(Core::Device* device, Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
        m_device = device;
        SetImmediateDestroyPolicy();

        m_processingItems.reserve(m_processingItems.get_container().capacity());

        m_thread = Threading::CreateThread(
            "AsyncCopyThread",
            [](const uintptr_t data) {
                reinterpret_cast<AsyncCopyQueue*>(data)->ThreadProc();
            },
            reinterpret_cast<uintptr_t>(this));

        m_threadEvent = Threading::Event::CreateManualReset();
        m_suspendEvent = Threading::Event::CreateManualReset();

        m_uploadBuffer = ImplCast(m_resourcePool->CreateByteAddressBuffer("AsyncUploadBuffer", kUploadBufferSize));
        m_resourcePool->CommitBufferMemory(m_uploadBuffer.Get(),
                                           { Core::BarrierAccessFlags::kCopySource | Core::BarrierAccessFlags::kCopyDest,
                                             Core::ResourceMemory::kHostWriteThrough });

        m_fence = Fence::Create(m_device, 0);

        VmaVirtualBlockCreateInfo virtualBlockCI = {};
        virtualBlockCI.size = kUploadBufferSize;
        virtualBlockCI.flags = VMA_VIRTUAL_BLOCK_CREATE_LINEAR_ALGORITHM_BIT;
        VerifyVk(vmaCreateVirtualBlock(&virtualBlockCI, &m_uploadRingBuffer));

        const auto* vkDevice = ImplCast(m_device);
        m_transferQueueFamilyIndex = vkDevice->GetQueueFamilyIndex(Core::DeviceQueueType::kTransfer);
        m_graphicsQueueFamilyIndex = vkDevice->GetQueueFamilyIndex(Core::DeviceQueueType::kGraphics);
        vkGetDeviceQueue(vkDevice->GetNative(), m_transferQueueFamilyIndex, 0, &m_queue);
    }


    AsyncCopyQueue::~AsyncCopyQueue()
    {
        {
            std::unique_lock lock{ m_suspendLock };
            m_exitRequested = true;
            m_suspendEvent.Reset();
            m_threadEvent.Send();
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
        VerifyVk(vkQueueWaitIdle(m_queue));
    }
} // namespace FE::Graphics::Vulkan
