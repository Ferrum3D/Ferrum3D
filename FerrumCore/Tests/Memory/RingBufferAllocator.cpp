#include <FeCore/Memory/RingBufferAllocator.h>
#include <Tests/Common/TestCommon.h>
#include <festd/vector.h>
#include <random>

using namespace FE;


TEST(RingBufferAllocator, AllocateFreeSequential)
{
    Memory::RingBufferAllocator allocator(256);

    const auto a = allocator.Allocate(1, 16);
    const auto b = allocator.Allocate(48, 16);
    const auto c = allocator.Allocate(16, 8);

    ASSERT_TRUE(a.IsValid());
    ASSERT_TRUE(b.IsValid());
    ASSERT_TRUE(c.IsValid());

    ASSERT_TRUE(IsAligned(a.m_offset, 16));
    ASSERT_TRUE(IsAligned(b.m_offset, 16));
    ASSERT_TRUE(IsAligned(c.m_offset, 8));

    const uint32_t used = allocator.GetUsedBytes();
    allocator.Free(a);
    allocator.Free(b);
    allocator.Free(c);

    EXPECT_EQ(allocator.GetUsedBytes(), 0u);
    EXPECT_EQ(allocator.GetStats().m_freeBytes, allocator.GetArenaSize());
    EXPECT_EQ(used, a.m_size + b.m_size + c.m_size);
}


TEST(RingBufferAllocator, BulkFree)
{
    Memory::RingBufferAllocator allocator(256);

    const auto a = allocator.Allocate(1, 16);
    const auto b = allocator.Allocate(48, 16);
    const auto c = allocator.Allocate(16, 8);

    const uint32_t used = allocator.GetUsedBytes();
    allocator.Free(used);

    EXPECT_EQ(allocator.GetUsedBytes(), 0u);
    EXPECT_EQ(allocator.GetStats().m_freeBytes, allocator.GetArenaSize());
    EXPECT_EQ(used, a.m_size + b.m_size + c.m_size);
}


TEST(RingBufferAllocator, WrapAround)
{
    Memory::RingBufferAllocator allocator(128);

    const auto first = allocator.Allocate(64, 16);
    const auto second = allocator.Allocate(32, 16);

    ASSERT_TRUE(first.IsValid());
    ASSERT_TRUE(second.IsValid());

    allocator.Free(first);

    const auto wrapped = allocator.Allocate(48, 16);
    ASSERT_TRUE(wrapped.IsValid());
    EXPECT_EQ(wrapped.m_offset, 0u);

    allocator.Free(second);
    allocator.Free(wrapped);

    const auto stats = allocator.GetStats();
    EXPECT_EQ(stats.m_freeBytes, allocator.GetArenaSize());
    EXPECT_EQ(stats.m_largestFreeBlock, allocator.GetArenaSize());
}


TEST(RingBufferAllocator, WrapAroundAndBulkFree)
{
    Memory::RingBufferAllocator allocator(128);

    const auto first = allocator.Allocate(64, 16);
    const auto second = allocator.Allocate(32, 16);

    ASSERT_TRUE(first.IsValid());
    ASSERT_TRUE(second.IsValid());

    allocator.Free(first);

    const auto wrapped = allocator.Allocate(48, 16);
    ASSERT_TRUE(wrapped.IsValid());
    EXPECT_EQ(wrapped.m_offset, 0u);

    allocator.Free(second.m_size + wrapped.m_size);

    const auto stats = allocator.GetStats();
    EXPECT_EQ(stats.m_freeBytes, allocator.GetArenaSize());
    EXPECT_EQ(stats.m_largestFreeBlock, allocator.GetArenaSize());
}


TEST(RingBufferAllocator, Exhaustion)
{
    Memory::RingBufferAllocator allocator(96);

    const auto a = allocator.Allocate(64, 16);
    const auto b = allocator.Allocate(32, 16);
    const auto c = allocator.Allocate(1, 1);

    ASSERT_TRUE(a.IsValid());
    ASSERT_TRUE(b.IsValid());
    EXPECT_FALSE(c.IsValid());

    allocator.Free(a);
    allocator.Free(b);
}


TEST(RingBufferAllocator, RandomSequentialFree)
{
    Memory::RingBufferAllocator allocator(2048);

    std::mt19937 rng(0);
    std::uniform_int_distribution<uint32_t> sizeDist(1, 128);
    std::uniform_int_distribution<uint32_t> alignmentPow(0, 6);

    festd::vector<Memory::RingBufferAllocator::Handle> allocations;
    for (;;)
    {
        const uint32_t size = sizeDist(rng);
        const uint32_t alignment = 1u << alignmentPow(rng);
        auto handle = allocator.Allocate(size, alignment);
        if (!handle.IsValid())
            break;

        EXPECT_TRUE(IsAligned(handle.m_offset, alignment));
        allocations.push_back(handle);
    }

    for (const auto& allocation : allocations)
    {
        allocator.Free(allocation);
        const auto stats = allocator.GetStats();
        EXPECT_LE(stats.m_freeBytes, allocator.GetArenaSize());
        EXPECT_LE(stats.m_largestFreeBlock, allocator.GetArenaSize());
    }

    const auto stats = allocator.GetStats();
    EXPECT_EQ(stats.m_freeBytes, allocator.GetArenaSize());
    EXPECT_EQ(stats.m_largestFreeBlock, allocator.GetArenaSize());
}
