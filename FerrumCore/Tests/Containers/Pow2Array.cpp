#include <Core/Containers/Pow2Array.h>
#include <gtest/gtest.h>

using namespace FE;

namespace
{
    struct CheckedAllocator final : public std::pmr::memory_resource
    {
        uint32_t m_allocationCount = 0;
        uint32_t m_deallocationCount = 0;

    private:
        void* do_allocate(const size_t byteSize, const size_t byteAlignment) override
        {
            EXPECT_GT(byteSize, 0);
            ++m_allocationCount;
            return Memory::DefaultAllocate(byteSize, byteAlignment);
        }

        void do_deallocate(void* pointer, const size_t byteSize, const size_t) override
        {
            EXPECT_NE(pointer, nullptr);
            EXPECT_GT(byteSize, 0);
            ++m_deallocationCount;
            Memory::DefaultFree(pointer);
        }

        [[nodiscard]] bool do_is_equal(const memory_resource& other) const noexcept override
        {
            return this == &other;
        }
    };
} // namespace


TEST(Pow2Array, FirstInsertionAndRemoval)
{
    CheckedAllocator allocator;
    {
        Pow2Array<uint32_t> values(&allocator);
        values.push_back(7);
        EXPECT_EQ(values.size(), 1);
        EXPECT_EQ(values[0], 7);

        values.erase_unsorted(values.begin());
        EXPECT_TRUE(values.empty());
    }

    EXPECT_EQ(allocator.m_allocationCount, allocator.m_deallocationCount);
}


TEST(Pow2Array, RemovalUpdatesMovedEntry)
{
    Pow2Array<uint32_t> values;
    values.push_back(1);
    values.push_back(2);
    values.push_back(3);

    values.erase_unsorted(values.begin());
    EXPECT_EQ(values.size(), 2);
    EXPECT_EQ(values[0], 3);
    EXPECT_EQ(values[1], 2);
}
