#include <Graphics/Assets/AssetStreamingOperation.h>
#include <Graphics/Core/Fence.h>
#include <gtest/gtest.h>

using namespace FE;

namespace
{
    struct CommandAllocator final : std::pmr::memory_resource
    {
        uint32_t m_liveAllocations = 0;

    private:
        void* do_allocate(size_t size, size_t alignment) override
        {
            ++m_liveAllocations;
            return Memory::DefaultAllocate(size, alignment);
        }


        void do_deallocate(void* pointer, size_t, size_t) override
        {
            --m_liveAllocations;
            Memory::DefaultFree(pointer);
        }


        bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
        {
            return this == &other;
        }
    };


    struct CopyQueue final : Graphics::Core::AsyncCopyQueue
    {
        uint32_t m_drains = 0;
        Graphics::Core::AsyncCopyCommandList* m_pending = nullptr;

        Graphics::Core::FenceSyncPoint GetCurrentFence() const override
        {
            return {};
        }


        void ExecuteCommandList(Graphics::Core::AsyncCopyCommandList* commands) override
        {
            m_pending = commands;
        }


        void Drain() override
        {
            ++m_drains;
            const Rc<WaitGroup> completion = m_pending->m_signalWaitGroup;
            m_pending->m_buffer.Free();
            Memory::Delete(m_pending->m_allocator, m_pending);
            m_pending = nullptr;
            completion->Signal();
        }

    private:
        void DestroyObject() override
        {
            Memory::DefaultDelete(this);
        }
    };


    struct PreparedOperation final : Graphics::AssetStreamingOperation
    {
        explicit PreparedOperation(CopyQueue& queue)
            : AssetStreamingOperation(Graphics::AssetStreamingOperationType::kStreamIn, 0, &queue)
        {
        }


        void Prepare(CommandAllocator& allocator)
        {
            Graphics::Core::AsyncCopyCommandListBuilder builder(&allocator, 4096);
            // Cancellation only discards this upload; no GPU resource is required.
            builder.UploadBuffer(nullptr, nullptr, 0, 0, 1);
            m_commandList = builder.Build(&allocator, m_uploadDone.Get());
            m_prepareSucceeded = true;
            m_prepareDone->Signal();
        }


        Graphics::AssetStreamingOperationStatus Tick() override
        {
            if (!m_uploadSubmitted)
            {
                m_asyncCopyQueue->ExecuteCommandList(m_commandList);
                m_commandList = nullptr;
                m_uploadSubmitted = true;
            }

            return Graphics::AssetStreamingOperationStatus::kPending;
        }


        void Cancel() override
        {
            CancelAndWait();
        }


        void Destroy() override
        {
            Memory::DefaultDelete(this);
        }
    };
} // namespace


TEST(AssetStreaming, CancelDiscardsPreparedUnsubmittedCommands)
{
    CommandAllocator allocator;
    CopyQueue queue;
    PreparedOperation operation(queue);
    operation.Prepare(allocator);
    ASSERT_GT(allocator.m_liveAllocations, 0);

    operation.Cancel();
    EXPECT_EQ(allocator.m_liveAllocations, 0);
    EXPECT_EQ(queue.m_drains, 0);
    EXPECT_EQ(queue.m_pending, nullptr);
    operation.Cancel();
    EXPECT_EQ(allocator.m_liveAllocations, 0);
}


TEST(AssetStreaming, CancelDrainsSubmittedUploadsBeforeAllocatorDestruction)
{
    CommandAllocator allocator;
    CopyQueue queue;
    PreparedOperation operation(queue);
    operation.Prepare(allocator);
    operation.Tick();
    ASSERT_NE(queue.m_pending, nullptr);

    operation.Cancel();
    EXPECT_EQ(allocator.m_liveAllocations, 0);
    EXPECT_EQ(queue.m_drains, 1);
    EXPECT_EQ(queue.m_pending, nullptr);
}
