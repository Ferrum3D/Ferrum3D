#include <FeCore/Base/Assert.h>
#include <FeCore/Base/BaseMath.h>
#include <FeCore/Memory/BuddyAllocator.h>

namespace FE::Memory
{
    uint32_t BuddyAllocator::Handle::GetSize() const
    {
        return BlockSizeForLevel(m_level);
    }


    void BuddyAllocator::Setup(const uint32_t arenaByteSize)
    {
        FE_Assert(m_arenaSize == 0);
        FE_Assert(arenaByteSize >= kMinBlockSize);
        FE_Assert(Math::IsPowerOfTwo(arenaByteSize));
        FE_Assert(IsAligned(arenaByteSize, kMinBlockSize));

        m_arenaSize = arenaByteSize;
        m_minBlockLog2 = Math::FloorLog2(kMinBlockSize);

        const uint32_t arenaLog2 = Math::FloorLog2(m_arenaSize);
        m_maxLevel = arenaLog2 - m_minBlockLog2;

        m_freeBits.resize(m_maxLevel + 1);

        for (uint32_t level = 0; level <= m_maxLevel; ++level)
        {
            const uint32_t blockSize = BlockSizeForLevel(level);
            const uint32_t blockCount = m_arenaSize / blockSize;
            m_freeBits[level].resize(blockCount, false);
        }

        m_freeBits[m_maxLevel].set(0);
    }


    BuddyAllocator::Handle BuddyAllocator::Allocate(const uint32_t size, uint32_t alignment)
    {
        Handle handle;
        if (size == 0)
            return handle;

        if (alignment == 0)
            alignment = kMinBlockSize;

        FE_AssertDebug(Math::IsPowerOfTwo(alignment));

        uint32_t request = Math::Max(size, alignment);
        request = Math::Max(request, kMinBlockSize);
        if (request > m_arenaSize)
            return handle;

        uint32_t blockSize = Math::CeilPowerOfTwo(request);
        blockSize = Math::Max(blockSize, kMinBlockSize);

        const uint32_t level = LevelForBlockSize(blockSize);
        if (level > m_maxLevel)
            return handle;

        uint32_t foundLevel = level;
        uint32_t blockIndex = 0;
        for (; foundLevel <= m_maxLevel; ++foundLevel)
        {
            const uint32_t foundIndex = m_freeBits[foundLevel].find_first();
            if (foundIndex != kInvalidIndex)
            {
                blockIndex = foundIndex;
                break;
            }
        }

        if (foundLevel > m_maxLevel)
            return handle;

        m_freeBits[foundLevel].reset(blockIndex);
        while (foundLevel > level)
        {
            --foundLevel;
            blockIndex *= 2u;
            m_freeBits[foundLevel].set(blockIndex + 1u);
        }

        handle.m_offset = blockIndex * blockSize;
        handle.m_level = static_cast<uint8_t>(level);
        return handle;
    }


    void BuddyAllocator::Free(const Handle handle)
    {
        if (!handle.IsValid())
            return;

        const uint32_t level = handle.m_level;
        FE_Assert(level <= m_maxLevel);

        const uint32_t blockSize = BlockSizeForLevel(level);
        FE_Assert(IsAligned(handle.m_offset, blockSize));

        uint32_t blockIndex = handle.m_offset / blockSize;
        FE_Assert(!m_freeBits[level].test(blockIndex), "Double free detected");

        uint32_t currentLevel = level;
        while (currentLevel < m_maxLevel)
        {
            const uint32_t buddyIndex = blockIndex ^ 1u;
            if (!m_freeBits[currentLevel].test(buddyIndex))
                break;

            m_freeBits[currentLevel].reset(buddyIndex);
            blockIndex >>= 1u;
            ++currentLevel;
        }

        m_freeBits[currentLevel].set(blockIndex);
    }


    BuddyAllocator::Stats BuddyAllocator::GetStats() const
    {
        Stats stats;

        for (uint32_t level = 0; level <= m_maxLevel; ++level)
        {
            const uint32_t blockSize = BlockSizeForLevel(level);
            const uint32_t freeBlocks = Bit::PopCount(m_freeBits[level].view());

            stats.m_freeBlocks += freeBlocks;
            stats.m_freeBytes += freeBlocks * blockSize;
            if (freeBlocks > 0)
                stats.m_largestFreeBlock = Math::Max(stats.m_largestFreeBlock, blockSize);
        }

        return stats;
    }


    uint32_t BuddyAllocator::BlockSizeForLevel(const uint32_t level)
    {
        return kMinBlockSize << level;
    }


    uint32_t BuddyAllocator::LevelForBlockSize(const uint32_t blockSize) const
    {
        return Math::FloorLog2(blockSize) - m_minBlockLog2;
    }
} // namespace FE::Memory
