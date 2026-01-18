#include <FeCore/Base/Assert.h>
#include <FeCore/Base/BaseMath.h>
#include <FeCore/Memory/RingBufferAllocator.h>

namespace FE::Memory
{
    RingBufferAllocator::RingBufferAllocator(const uint32_t arenaByteSize)
        : m_capacity(arenaByteSize)
    {
        FE_CoreAssert(m_capacity > 0);
    }


    RingBufferAllocator::Handle RingBufferAllocator::Allocate(uint32_t size, uint32_t alignment)
    {
        Handle handle;
        if (size == 0 || m_capacity == 0)
            return handle;

        if (alignment == 0)
            alignment = 1;

        FE_CoreAssert(Math::IsPowerOfTwo(alignment));

        uint32_t freeBytes = m_capacity - m_used;
        if (freeBytes < size)
            return handle;

        uint32_t tail = m_tail;
        uint32_t alignedTail = AlignUp(tail, alignment);
        uint32_t padding = alignedTail - tail;
        uint32_t total = padding + size;

        if (m_tail >= m_head)
        {
            if (alignedTail + size <= m_capacity)
            {
                if (total > freeBytes)
                    return handle;
            }
            else
            {
                if (m_wrapPadding != 0)
                    return handle;

                const uint32_t wrapPadding = m_capacity - m_tail;
                if (wrapPadding > freeBytes)
                    return handle;

                const uint32_t freeAfterPadding = freeBytes - wrapPadding;
                if (m_head == 0 || size > freeAfterPadding)
                    return handle;

                m_wrapBoundary = m_tail;
                m_wrapPadding = wrapPadding;
                m_used += wrapPadding;

                m_tail = 0;
                alignedTail = 0;
                total = size;
            }
        }
        else
        {
            if (alignedTail + size > m_head)
                return handle;

            if (total > freeBytes)
                return handle;
        }

        m_used += total;
        m_tail = alignedTail + size;

        handle.m_offset = alignedTail;
        handle.m_size = total;
        return handle;
    }


    void RingBufferAllocator::Free(const Handle handle)
    {
        if (!handle.IsValid())
            return;

        FE_CoreAssert(handle.m_size <= m_used);

        m_head += handle.m_size;
        m_used -= handle.m_size;

        if (m_wrapPadding != 0 && m_head == m_wrapBoundary)
        {
            m_head = 0;
            m_used -= m_wrapPadding;
            m_wrapPadding = 0;
            m_wrapBoundary = 0;
        }

        if (m_head == m_capacity)
            m_head = 0;

        if (m_used == 0)
        {
            m_head = 0;
            m_tail = 0;
            m_wrapPadding = 0;
            m_wrapBoundary = 0;
        }
    }


    RingBufferAllocator::Stats RingBufferAllocator::GetStats() const
    {
        Stats stats;
        stats.m_freeBytes = m_capacity - m_used;

        if (m_used == 0)
        {
            stats.m_freeBlocks = 1;
            stats.m_largestFreeBlock = m_capacity;
            return stats;
        }

        if (m_tail >= m_head)
        {
            const uint32_t freeEnd = m_capacity - m_tail;
            const uint32_t freeStart = m_head;
            stats.m_freeBlocks = (freeEnd > 0 ? 1u : 0u) + (freeStart > 0 ? 1u : 0u);
            stats.m_largestFreeBlock = Math::Max(freeEnd, freeStart);
        }
        else
        {
            const uint32_t freeMid = m_head - m_tail;
            stats.m_freeBlocks = freeMid > 0 ? 1u : 0u;
            stats.m_largestFreeBlock = freeMid;
        }

        return stats;
    }
} // namespace FE::Memory
