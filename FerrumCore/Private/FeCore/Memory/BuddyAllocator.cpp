#include <FeCore/Base/Assert.h>
#include <FeCore/Base/BaseMath.h>
#include <FeCore/Memory/BuddyAllocator.h>

namespace FE::Memory
{
    uint32_t BuddyAllocator::Handle::GetSize() const
    {
        return BlockSizeForOrder(m_order);
    }


    BuddyAllocator::BuddyAllocator(const uint32_t arenaByteSize)
    {
        FE_CoreAssert(arenaByteSize >= kMinBlockSize);
        FE_CoreAssert(Math::IsPowerOfTwo(arenaByteSize));
        FE_CoreAssert(IsAligned(arenaByteSize, kMinBlockSize));

        m_arenaSize = arenaByteSize;
        m_minBlockLog2 = Math::FloorLog2(kMinBlockSize);

        const uint32_t arenaLog2 = Math::FloorLog2(m_arenaSize);
        m_maxOrder = arenaLog2 - m_minBlockLog2;

        m_freeBits.resize(m_maxOrder + 1);

        for (uint32_t order = 0; order <= m_maxOrder; ++order)
        {
            const uint32_t blockSize = BlockSizeForOrder(order);
            const uint32_t blockCount = m_arenaSize / blockSize;
            m_freeBits[order].resize(blockCount, false);
        }

        m_freeBits[m_maxOrder].set(0);
    }


    BuddyAllocator::Handle BuddyAllocator::Allocate(const uint32_t size, uint32_t alignment)
    {
        Handle handle;
        if (size == 0)
            return handle;

        if (alignment == 0)
            alignment = kMinBlockSize;

        FE_CoreAssert(Math::IsPowerOfTwo(alignment));

        uint32_t request = Math::Max(size, alignment);
        request = Math::Max(request, kMinBlockSize);
        if (request > m_arenaSize)
            return handle;

        uint32_t blockSize = Math::CeilPowerOfTwo(request);
        blockSize = Math::Max(blockSize, kMinBlockSize);

        const uint32_t order = OrderForBlockSize(blockSize);
        if (order > m_maxOrder)
            return handle;

        uint32_t foundOrder = order;
        uint32_t blockIndex = 0;
        for (; foundOrder <= m_maxOrder; ++foundOrder)
        {
            const uint32_t foundIndex = m_freeBits[foundOrder].find_first();
            if (foundIndex != kInvalidIndex)
            {
                blockIndex = foundIndex;
                break;
            }
        }

        if (foundOrder > m_maxOrder)
            return handle;

        m_freeBits[foundOrder].reset(blockIndex);
        while (foundOrder > order)
        {
            --foundOrder;
            blockIndex *= 2u;
            m_freeBits[foundOrder].set(blockIndex + 1u);
        }

        handle.m_offset = blockIndex * blockSize;
        handle.m_order = static_cast<uint8_t>(order);
        return handle;
    }


    void BuddyAllocator::Free(const Handle handle)
    {
        if (!handle.IsValid())
            return;

        const uint32_t order = handle.m_order;
        FE_CoreAssert(order <= m_maxOrder);

        const uint32_t blockSize = BlockSizeForOrder(order);
        FE_CoreAssert(IsAligned(handle.m_offset, blockSize));

        uint32_t blockIndex = handle.m_offset / blockSize;
        FE_CoreAssert(!m_freeBits[order].test(blockIndex), "Double free detected");

        uint32_t currentOrder = order;
        while (currentOrder < m_maxOrder)
        {
            const uint32_t buddyIndex = blockIndex ^ 1u;
            if (!m_freeBits[currentOrder].test(buddyIndex))
                break;

            m_freeBits[currentOrder].reset(buddyIndex);
            blockIndex >>= 1u;
            ++currentOrder;
        }

        m_freeBits[currentOrder].set(blockIndex);
    }


    BuddyAllocator::Stats BuddyAllocator::GetStats() const
    {
        Stats stats;

        for (uint32_t order = 0; order <= m_maxOrder; ++order)
        {
            const uint32_t blockSize = BlockSizeForOrder(order);
            const uint32_t freeBlocks = Bit::PopCount(m_freeBits[order].view());

            stats.m_freeBlocks += freeBlocks;
            stats.m_freeBytes += freeBlocks * blockSize;
            if (freeBlocks > 0)
                stats.m_largestFreeBlock = Math::Max(stats.m_largestFreeBlock, blockSize);
        }

        return stats;
    }


    uint32_t BuddyAllocator::BlockSizeForOrder(const uint32_t order)
    {
        return kMinBlockSize << order;
    }


    uint32_t BuddyAllocator::OrderForBlockSize(const uint32_t blockSize) const
    {
        return Math::FloorLog2(blockSize) - m_minBlockLog2;
    }
} // namespace FE::Memory
