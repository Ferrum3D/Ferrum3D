#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <FeCore/Threading/Event.h>
#include <FeCore/Threading/Thread.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/Fence.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <festd/ring_buffer.h>

namespace FE::Graphics::Vulkan
{
    struct AsyncCopyQueue final : public Core::AsyncCopyQueue
    {
        FE_RTTI_Class(AsyncCopyQueue, "73A6B2DA-9BD1-421E-B27F-AA09DA277F42");

        AsyncCopyQueue(Core::Device* device, Core::ResourcePool* resourcePool);
        ~AsyncCopyQueue() override;

        void ExecuteCommandList(Core::AsyncCopyCommandList* commandList) override;
        void Drain() override;

    private:
        static constexpr uint32_t kStagingAllocationAlignment = 256;
        static constexpr uint32_t kUploadBufferSize = 4 * 1024 * 1024;

        struct ProcessingItem final
        {
            Core::AsyncCopyCommandList m_queueItem;
            Rc<CommandBuffer> m_commandBuffer;
            uint64_t m_fenceValue = 0;

            festd::small_vector<VmaVirtualAllocation> m_stagingAllocations;
        };

        void ThreadProc();
        bool FinalizeFinishedProcessors(bool wait = false);
        void ProcessCommandList(ProcessingItem* item);
        VkDeviceSize AllocateStagingMemory(ProcessingItem* item, size_t byteSize, size_t byteAlignment);
        void SubmitCommandList(VkCommandBuffer commandBuffer, uint64_t fenceValue) const;
        Rc<CommandBuffer> AcquireCommandBuffer();

        Core::ResourcePool* m_resourcePool = nullptr;

        Threading::ThreadHandle m_thread;
        Threading::Event m_threadEvent;
        Threading::Event m_suspendEvent;
        std::atomic<bool> m_exitRequested = false;
        Threading::SharedSpinLock m_suspendLock;

        VkQueue m_queue = VK_NULL_HANDLE;
        uint32_t m_transferQueueFamilyIndex = kInvalidIndex;
        uint32_t m_graphicsQueueFamilyIndex = kInvalidIndex;
        Rc<Buffer> m_uploadBuffer;
        VmaVirtualBlock m_uploadRingBuffer = VK_NULL_HANDLE;
        uint64_t m_fenceValue = 0;
        Rc<Core::Fence> m_fence;

        uint32_t m_commandBufferCounter = 0;
        festd::small_vector<Rc<CommandBuffer>> m_freeCommandBuffers;
        festd::small_ring_buffer<ProcessingItem*, 32> m_processingItems;
        Memory::Pool<ProcessingItem> m_processingItemPool{ "AsyncCopyProcessingItemPool" };

        ConcurrentOnceConsumedQueue m_requestQueue;
        festd::small_vector<Core::AsyncCopyCommandList*> m_requestCache;
        Memory::LinearAllocator m_threadTempAllocator;
    };
} // namespace FE::Graphics::Vulkan
