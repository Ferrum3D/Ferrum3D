#include <FeCore/Memory/BuddyAllocator.h>
#include <Tests/Common/TestCommon.h>
#include <algorithm>
#include <random>

using namespace FE;

TEST(BuddyAllocator, AllocateFree)
{
    Memory::BuddyAllocator allocator(256);

    auto handle = allocator.Allocate(1, 1);
    ASSERT_TRUE(handle.IsValid());
    EXPECT_EQ(handle.m_offset % 16u, 0u);
    EXPECT_EQ(handle.GetSize(), 16u);

    allocator.Free(handle);

    auto full = allocator.Allocate(256, 16);
    ASSERT_TRUE(full.IsValid());
    EXPECT_EQ(full.m_offset, 0u);
    EXPECT_EQ(full.GetSize(), 256u);
    allocator.Free(full);
}

TEST(BuddyAllocator, SplitAndCoalesce)
{
    Memory::BuddyAllocator allocator(256);

    auto first = allocator.Allocate(16, 16);
    auto second = allocator.Allocate(16, 16);

    ASSERT_TRUE(first.IsValid());
    ASSERT_TRUE(second.IsValid());

    allocator.Free(first);
    allocator.Free(second);

    auto merged = allocator.Allocate(32, 16);
    ASSERT_TRUE(merged.IsValid());
    EXPECT_EQ(merged.m_offset, 0u);
    EXPECT_EQ(merged.GetSize(), 32u);
    allocator.Free(merged);
}

TEST(BuddyAllocator, Alignment)
{
    Memory::BuddyAllocator allocator(1024);

    auto handle = allocator.Allocate(24, 64);
    ASSERT_TRUE(handle.IsValid());
    EXPECT_EQ(handle.m_offset % 64u, 0u);
    EXPECT_EQ(handle.GetSize(), 64u);
    allocator.Free(handle);
}

TEST(BuddyAllocator, ExhaustionAndReuse)
{
    Memory::BuddyAllocator allocator(256);

    festd::vector<Memory::BuddyAllocator::Handle> handles;
    handles.reserve(16);

    for (uint32_t i = 0; i < 16; ++i)
    {
        auto handle = allocator.Allocate(1, 1);
        ASSERT_TRUE(handle.IsValid());
        handles.push_back(handle);
    }

    auto failed = allocator.Allocate(1, 1);
    EXPECT_FALSE(failed.IsValid());

    for (const auto handle : handles)
        allocator.Free(handle);

    auto full = allocator.Allocate(256, 16);
    ASSERT_TRUE(full.IsValid());
    EXPECT_EQ(full.m_offset, 0u);
    allocator.Free(full);
}

TEST(BuddyAllocator, RandomAllocations)
{
    Memory::BuddyAllocator allocator(64 * 1024);

    struct Allocation
    {
        Memory::BuddyAllocator::Handle m_handle;
        uint32_t m_blockSize = 0;
    };

    std::mt19937 rng(0);
    std::uniform_int_distribution<uint32_t> sizeDist(1, 256);
    std::uniform_int_distribution<uint32_t> alignmentPow(0, 6);

    auto validateStats = [&](uint32_t allocatedBytes) {
        const auto stats = allocator.GetStats();
        EXPECT_EQ(stats.m_freeBytes + allocatedBytes, allocator.GetArenaSize());
        EXPECT_LE(stats.m_largestFreeBlock, allocator.GetArenaSize());
        EXPECT_LE(stats.m_freeBytes, allocator.GetArenaSize());
    };

    festd::vector<Allocation> allocations;
    uint32_t allocatedBytes = 0;

    for (;;)
    {
        const uint32_t size = sizeDist(rng);
        const uint32_t alignment = 1u << alignmentPow(rng);
        auto handle = allocator.Allocate(size, alignment);
        if (!handle.IsValid())
            break;

        const uint32_t blockSize = handle.GetSize();
        allocatedBytes += blockSize;
        allocations.push_back({ handle, blockSize });
    }

    std::shuffle(allocations.begin(), allocations.end(), rng);

    const uint32_t toFree = allocations.size() / 2;
    for (uint32_t i = 0; i < toFree; ++i)
    {
        allocatedBytes -= allocations[i].m_blockSize;
        allocator.Free(allocations[i].m_handle);
        validateStats(allocatedBytes);
    }

    allocations.erase(allocations.begin(), allocations.begin() + toFree);

    for (;;)
    {
        const uint32_t size = sizeDist(rng);
        const uint32_t alignment = 1u << alignmentPow(rng);
        auto handle = allocator.Allocate(size, alignment);
        if (!handle.IsValid())
            break;

        const uint32_t blockSize = handle.GetSize();
        allocatedBytes += blockSize;
        allocations.push_back({ handle, blockSize });
    }

    std::shuffle(allocations.begin(), allocations.end(), rng);
    for (const auto& allocation : allocations)
    {
        allocatedBytes -= allocation.m_blockSize;
        allocator.Free(allocation.m_handle);
        validateStats(allocatedBytes);
    }

    const auto stats = allocator.GetStats();
    EXPECT_EQ(stats.m_freeBytes, allocator.GetArenaSize());
    EXPECT_EQ(stats.m_largestFreeBlock, allocator.GetArenaSize());
    EXPECT_EQ(stats.m_freeBlocks, 1u);
}
