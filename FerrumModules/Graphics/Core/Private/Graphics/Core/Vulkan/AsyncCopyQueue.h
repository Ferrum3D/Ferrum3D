#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/Jobs/Job.h>
#include <FeCore/Threading/Event.h>
#include <FeCore/Threading/Thread.h>
#include <Graphics/Core/AsyncCopyQueue.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <festd/bit_vector.h>

namespace FE::Graphics::Vulkan
{
    struct AsyncCopyQueue final : public Core::AsyncCopyQueue
    {
        FE_RTTI_Class(AsyncCopyQueue, "73A6B2DA-9BD1-421E-B27F-AA09DA277F42");

        AsyncCopyQueue(Core::Device* device, Core::ResourcePool* resourcePool);
        ~AsyncCopyQueue() override;

        Rc<WaitGroup> ExecuteCommandList(const Core::AsyncCopyCommandList& commandList) override;
        void Drain() override;

    private:
        static constexpr uint32_t kMaxInFlightSubmits = 8;
        static constexpr uint32_t kStagingAllocationAlignment = 256;
        static constexpr uint32_t kUploadBufferSize = 4 * 1024 * 1024;

        void ThreadProc();
        bool FinalizeFinishedProcessors();
        void ProcessCommandList(uint32_t processorIndex);

        Core::ResourcePool* m_resourcePool = nullptr;

        Threading::ThreadHandle m_thread;
        Threading::Event m_threadEvent;
        std::atomic<bool> m_threadRunning = false;

        VkQueue m_queue = VK_NULL_HANDLE;
        Rc<Buffer> m_uploadBuffer;
        VmaVirtualBlock m_uploadBufferBlock = VK_NULL_HANDLE;

        struct Slot final
        {
            Rc<WaitGroup> m_waitGroup;
            Core::AsyncCopyCommandList m_commandList;
        };

        struct InFlightProcessor final
        {
            VmaVirtualAllocation m_stagingAllocation = VK_NULL_HANDLE;
            VkFence m_fence = VK_NULL_HANDLE;
            Rc<CommandBuffer> m_commandBuffer;
            uint32_t m_slotIndex = kInvalidIndex;
            bool m_isProcessing = false;
            bool m_isQueued = false;
        };

        festd::array<InFlightProcessor, kMaxInFlightSubmits> m_inFlightProcessors;
        uint32_t m_busyProcessorCount = 0;

        Threading::SpinLock m_lock;
        festd::bit_vector m_freeSlots;
        festd::bit_vector m_queuedSlots;
        SegmentedVector<Slot> m_slots;
    };
} // namespace FE::Graphics::Vulkan
