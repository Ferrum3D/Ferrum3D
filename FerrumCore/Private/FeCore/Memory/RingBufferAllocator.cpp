#include <FeCore/Base/Assert.h>
#include <FeCore/Base/BaseMath.h>
#include <FeCore/Memory/RingBufferAllocator.h>

namespace FE::Memory
{
    void RingBufferAllocator::Setup(const uint32_t capacity)
    {
        FE_Assert(m_head == 0);
        FE_Assert(m_used == 0);
        FE_Assert(capacity > 0);

        m_capacity = capacity;
    }


    RingBufferAllocator::Handle RingBufferAllocator::Allocate(const uint32_t size, uint32_t alignment)
    {
        if (size == 0 || m_capacity == 0)
            return kInvalidHandle;

        if (alignment == 0)
            alignment = 1;

        FE_AssertDebug(Math::IsPowerOfTwo(alignment));

        const uint32_t tail = GetTail();
        const uint32_t alignmentPadding = AlignUp(tail, alignment) - tail;
        const uint32_t wrapPadding = CalculateWrapPadding(tail, size + alignmentPadding);

        const uint32_t freeSize = m_capacity - m_used;
        const uint32_t padding = wrapPadding > 0 ? wrapPadding : alignmentPadding;
        if (freeSize < padding + size)
            return kInvalidHandle;

        m_used += padding;

        Handle handle;
        handle.m_offset = GetTail();
        handle.m_size = size + padding;

        m_used += size;

        return handle;
    }


    void RingBufferAllocator::Free(const Handle handle)
    {
        if (!handle.IsValid())
            return;

        Free(handle.m_size);
    }


    void RingBufferAllocator::Free(const uint32_t bytes)
    {
        FE_Assert(bytes <= m_used);

        m_head = (m_head + bytes) % m_capacity;
        m_used -= bytes;

        if (m_used == 0)
            m_head = 0;
    }


    RingBufferAllocator::Stats RingBufferAllocator::GetStats() const
    {
        Stats stats;
        stats.m_freeBytes = m_capacity - m_used;

        if (m_used == 0)
        {
            stats.m_largestFreeBlock = m_capacity;
            return stats;
        }

        const uint32_t tail = GetTail();
        if (tail >= m_head)
        {
            const uint32_t freeEnd = m_capacity - tail;
            const uint32_t freeStart = m_head;
            stats.m_largestFreeBlock = Math::Max(freeEnd, freeStart);
        }
        else
        {
            const uint32_t freeMid = m_head - tail;
            stats.m_largestFreeBlock = freeMid;
        }

        return stats;
    }


    uint32_t RingBufferAllocator::GetTail() const
    {
        return (m_head + m_used) % m_capacity;
    }


    uint32_t RingBufferAllocator::CalculateWrapPadding(const uint32_t tail, const uint32_t size) const
    {
        if (m_used != m_capacity && tail + size > m_capacity)
            return m_capacity - tail;

        return 0;
    }
} // namespace FE::Memory
