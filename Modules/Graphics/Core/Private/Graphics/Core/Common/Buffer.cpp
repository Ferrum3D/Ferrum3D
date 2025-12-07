#include <Graphics/Core/Common/Buffer.h>

namespace FE::Graphics::Common
{
    Core::ResourceMemory Buffer::GetMemoryStatus() const
    {
        if (m_instance == nullptr)
            return Core::ResourceMemory::kNotCommitted;

        return m_instance->m_memoryStatus;
    }


    void Buffer::SetState(const SubresourceState state)
    {
        std::unique_lock lk{ m_lock };
        festd::single(m_instance->m_subresourceStates) = state;
    }


    SubresourceState Buffer::GetState() const
    {
        std::unique_lock lk{ m_lock };
        return festd::single(m_instance->m_subresourceStates);
    }


    void Buffer::AddQueueReleaseBarrier(const Core::BufferBarrierDesc& barrier)
    {
        std::unique_lock lk{ m_lock };

        auto& releaseBarrier = m_queueReleaseBarriers[festd::to_underlying(barrier.m_queueAfter)];
        FE_Assert(!releaseBarrier.has_value());
        releaseBarrier = barrier;
    }


    festd::optional<Core::BufferBarrierDesc> Buffer::RetrieveQueueReleaseBarrier(const Core::DeviceQueueType receiverQueue)
    {
        std::unique_lock lk{ m_lock };

        return m_queueReleaseBarriers[festd::to_underlying(receiverQueue)];
    }
} // namespace FE::Graphics::Common
