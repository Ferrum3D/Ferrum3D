#include <Tests/Common/TestCommon.h>
#include <festd/bit_vector.h>
#include <festd/vector.h>
#include <random>

using namespace FE;

namespace
{
    BitSetWord* AllocateGuardedWords(size_t count)
    {
        const size_t pageSize = Memory::GetPlatformSpec().m_granularity;
        const size_t byteSize = AlignUp(count * sizeof(BitSetWord), pageSize);
        const size_t totalWords = byteSize / sizeof(BitSetWord);
        const size_t padWords = totalWords - count;

        FE_Assert(totalWords * sizeof(BitSetWord) == byteSize);

        void* memory = Memory::AllocateVirtual(byteSize + pageSize);
        Memory::ProtectVirtual(static_cast<std::byte*>(memory) + byteSize, pageSize, Memory::ProtectFlags::kNone);
        BitSetWord* result = static_cast<BitSetWord*>(memory) + padWords;
        if (padWords > 0)
            result[-1] = DefaultHash(&count, sizeof(count));
        return result;
    }


    void FreeGuardedWords(BitSetWord* words, size_t count)
    {
        const size_t pageSize = Memory::GetPlatformSpec().m_granularity;
        const size_t byteSize = AlignUp(count * sizeof(BitSetWord), pageSize);
        const size_t totalWords = byteSize / sizeof(BitSetWord);
        const size_t padWords = totalWords - count;

        void* memory = AlignDownPtr(words, pageSize);
        const BitSetWord* result = static_cast<BitSetWord*>(memory) + padWords;
        if (padWords > 0)
            EXPECT_EQ(result[-1], DefaultHash(&count, sizeof(count)));
        Memory::FreeVirtual(memory, byteSize);
    }
} // namespace


TEST(BitStorageUtils, CalculateWordCount)
{
    using namespace Internal;

    for (uint32_t bitCount = 0; bitCount < 100'000; ++bitCount)
    {
        const uint32_t wordCount = CalculateNextLevelWordCount(bitCount);
        const uint32_t wordIndex = CalculateWordIndex(bitCount);
        const uint32_t topLevelWordCount = CalculateNextLevelWordCount(wordCount);

        ASSERT_EQ(wordCount, Math::CeilDivide(bitCount, kBitSetBitsPerWord));
        ASSERT_EQ(wordIndex, bitCount / kBitSetBitsPerWord);
        ASSERT_EQ(topLevelWordCount, Math::CeilDivide(wordCount, kBitSetBitsPerWord));
        ASSERT_EQ(topLevelWordCount, Math::CeilDivide(bitCount, kBitSetBitsPerWord * kBitSetBitsPerWord));
    }
}

TEST(FixedBitSetStorage, Capacity)
{
    using namespace Internal;

#define TEST_CAPACITY(bitCount)                                                                                                  \
    do                                                                                                                           \
    {                                                                                                                            \
        FixedBitSetStorage<(bitCount)> storage;                                                                                  \
        ASSERT_GE(storage.CapacityImpl(), (bitCount));                                                                           \
    }                                                                                                                            \
    while (0)

    TEST_CAPACITY(1);
    TEST_CAPACITY(2);
    TEST_CAPACITY(3);
    TEST_CAPACITY(30);
    TEST_CAPACITY(31);
    TEST_CAPACITY(32);
    TEST_CAPACITY(33);
    TEST_CAPACITY(62);
    TEST_CAPACITY(63);
    TEST_CAPACITY(64);
    TEST_CAPACITY(65);
    TEST_CAPACITY(1022);
    TEST_CAPACITY(1023);
    TEST_CAPACITY(1024);
    TEST_CAPACITY(1025);
    TEST_CAPACITY(4094);
    TEST_CAPACITY(4095);
    TEST_CAPACITY(4096);
    TEST_CAPACITY(4097);

#undef TEST_CAPACITY
}

TEST(FixedBitSetStorage, Resize)
{
    using namespace Internal;

    FixedBitSetStorage<256> storage;
    EXPECT_GE(storage.CapacityImpl(), 256);
    EXPECT_EQ(storage.SizeImpl(), 0);

    storage.ResizeImpl(256, nullptr);
    EXPECT_EQ(storage.SizeImpl(), 256);
}

TEST(DynamicBitSetStorage, Initialize)
{
    using namespace Internal;

    TestAllocator allocator;

    auto memLeakGuard = festd::defer([&allocator] {
        ASSERT_EQ(allocator.m_allocationCount, allocator.m_deallocationCount);
        ASSERT_EQ(allocator.m_totalSize, 0);
    });

    DynamicBitSetStorage storage;
    EXPECT_EQ(storage.CapacityImpl(), 0);
    EXPECT_EQ(storage.SizeImpl(), 0);
    EXPECT_EQ(allocator.m_allocationCount, 0);

    storage.InitializeImpl(5000, &allocator);
    EXPECT_GE(storage.CapacityImpl(), 5000);
    EXPECT_EQ(storage.SizeImpl(), 5000);
    EXPECT_EQ(allocator.m_allocationCount, 1);

    storage.ReinitializeImpl(5000, &allocator);
    EXPECT_GE(storage.CapacityImpl(), 5000);
    EXPECT_EQ(storage.SizeImpl(), 5000);
    EXPECT_EQ(allocator.m_allocationCount, 2);
    EXPECT_EQ(allocator.m_deallocationCount, 1);

    storage.ReinitializeImpl(100, &allocator);
    EXPECT_GE(storage.CapacityImpl(), 100);
    EXPECT_EQ(storage.SizeImpl(), 100);
    EXPECT_EQ(allocator.m_allocationCount, 3);

    storage.ReinitializeImpl(1000, &allocator);
    EXPECT_GE(storage.CapacityImpl(), 1000);
    EXPECT_EQ(storage.SizeImpl(), 1000);
    EXPECT_EQ(allocator.m_allocationCount, 4);

    storage.DestroyImpl(&allocator);
    EXPECT_EQ(allocator.m_allocationCount, 4);
}

TEST(DynamicBitSetStorage, Reserve)
{
    using namespace Internal;

    TestAllocator allocator;

    auto memLeakGuard = festd::defer([&allocator] {
        ASSERT_EQ(allocator.m_allocationCount, allocator.m_deallocationCount);
        ASSERT_EQ(allocator.m_totalSize, 0);
    });

    DynamicBitSetStorage storage;

    storage.InitializeImpl(5000, &allocator);
    EXPECT_GE(storage.CapacityImpl(), 5000);
    EXPECT_EQ(storage.SizeImpl(), 5000);
    EXPECT_EQ(allocator.m_allocationCount, 1);

    const uint32_t wordCount = CalculateNextLevelWordCount(storage.SizeImpl());
    const uint32_t topLevelWordCount = CalculateNextLevelWordCount(wordCount);

    for (uint32_t wordIndex = 0; wordIndex < wordCount; ++wordIndex)
    {
        storage.WordsDataImpl()[wordIndex] = wordIndex;
    }

    for (uint32_t wordIndex = 0; wordIndex < topLevelWordCount; ++wordIndex)
    {
        storage.TopLevelDataImpl()[wordIndex] = wordIndex;
    }

    const BitSetWord* prevData = storage.WordsDataImpl();

    storage.ReserveImpl(10000, &allocator);
    EXPECT_GE(storage.CapacityImpl(), 10000);
    EXPECT_EQ(storage.SizeImpl(), 5000);
    EXPECT_EQ(allocator.m_allocationCount, 2);
    EXPECT_NE(storage.WordsDataImpl(), prevData);

    for (uint32_t wordIndex = 0; wordIndex < wordCount; ++wordIndex)
    {
        ASSERT_EQ(storage.WordsDataImpl()[wordIndex], wordIndex);
    }

    for (uint32_t wordIndex = 0; wordIndex < topLevelWordCount; ++wordIndex)
    {
        ASSERT_EQ(storage.TopLevelDataImpl()[wordIndex], wordIndex);
    }

    storage.DestroyImpl(&allocator);
}

TEST(DynamicBitSetStorage, Resize)
{
    using namespace Internal;

    TestAllocator allocator;

    auto memLeakGuard = festd::defer([&allocator] {
        ASSERT_EQ(allocator.m_allocationCount, allocator.m_deallocationCount);
        ASSERT_EQ(allocator.m_totalSize, 0);
    });

    DynamicBitSetStorage storage;

    storage.InitializeImpl(5000, &allocator);
    EXPECT_GE(storage.CapacityImpl(), 5000);
    EXPECT_EQ(storage.SizeImpl(), 5000);
    EXPECT_EQ(allocator.m_allocationCount, 1);

    const uint32_t wordCount = CalculateNextLevelWordCount(storage.SizeImpl());
    const uint32_t topLevelWordCount = CalculateNextLevelWordCount(wordCount);

    for (uint32_t wordIndex = 0; wordIndex < wordCount; ++wordIndex)
    {
        storage.WordsDataImpl()[wordIndex] = wordIndex;
    }

    for (uint32_t wordIndex = 0; wordIndex < topLevelWordCount; ++wordIndex)
    {
        storage.TopLevelDataImpl()[wordIndex] = wordIndex;
    }

    const BitSetWord* prevData = storage.WordsDataImpl();

    storage.ResizeImpl(10000, &allocator);
    EXPECT_GE(storage.CapacityImpl(), 10000);
    EXPECT_EQ(storage.SizeImpl(), 10000);
    EXPECT_EQ(allocator.m_allocationCount, 2);
    EXPECT_NE(storage.WordsDataImpl(), prevData);

    for (uint32_t wordIndex = 0; wordIndex < wordCount; ++wordIndex)
    {
        ASSERT_EQ(storage.WordsDataImpl()[wordIndex], wordIndex);
    }

    for (uint32_t wordIndex = 0; wordIndex < topLevelWordCount; ++wordIndex)
    {
        ASSERT_EQ(storage.TopLevelDataImpl()[wordIndex], wordIndex);
    }

    storage.DestroyImpl(&allocator);
}

TEST(DynamicBitSetStorage, Shrink)
{
    using namespace Internal;

    TestAllocator allocator;

    auto memLeakGuard = festd::defer([&allocator] {
        ASSERT_EQ(allocator.m_allocationCount, allocator.m_deallocationCount);
        ASSERT_EQ(allocator.m_totalSize, 0);
    });

    DynamicBitSetStorage storage;
    EXPECT_EQ(allocator.m_allocationCount, 0);
    storage.InitializeImpl(5000, &allocator);
    EXPECT_EQ(allocator.m_allocationCount, 1);
    storage.ReserveImpl(20000, &allocator);
    EXPECT_EQ(allocator.m_allocationCount, 2);

    const uint32_t wordCount = CalculateNextLevelWordCount(storage.SizeImpl());
    const uint32_t topLevelWordCount = CalculateNextLevelWordCount(wordCount);

    for (uint32_t wordIndex = 0; wordIndex < wordCount; ++wordIndex)
    {
        storage.WordsDataImpl()[wordIndex] = wordIndex;
    }

    for (uint32_t wordIndex = 0; wordIndex < topLevelWordCount; ++wordIndex)
    {
        storage.TopLevelDataImpl()[wordIndex] = wordIndex;
    }

    const BitSetWord* prevData = storage.WordsDataImpl();

    storage.ShrinkImpl(&allocator);
    EXPECT_GE(storage.CapacityImpl(), 5000);
    EXPECT_LT(storage.CapacityImpl(), 20000);
    EXPECT_EQ(storage.SizeImpl(), 5000);
    EXPECT_EQ(allocator.m_allocationCount, 3);
    EXPECT_NE(storage.WordsDataImpl(), prevData);

    for (uint32_t wordIndex = 0; wordIndex < wordCount; ++wordIndex)
    {
        ASSERT_EQ(storage.WordsDataImpl()[wordIndex], wordIndex);
    }

    for (uint32_t wordIndex = 0; wordIndex < topLevelWordCount; ++wordIndex)
    {
        ASSERT_EQ(storage.TopLevelDataImpl()[wordIndex], wordIndex);
    }

    storage.DestroyImpl(&allocator);
}

TEST(SetBitsInRange, SingleWord)
{
    using namespace Internal;

    BitSetWord* memory = AllocateGuardedWords(1);
    auto freeMemory = festd::defer([memory] {
        FreeGuardedWords(memory, 1);
    });

    const festd::span words{ memory, 1 };

    SetAllBitsInRange(words, 0, 0);
    EXPECT_EQ(words[0], 0);
    SetAllBitsInRange(words, 5, 0);
    EXPECT_EQ(words[0], 0);
    SetAllBitsInRange(words, kBitSetBitsPerWord, 0);
    EXPECT_EQ(words[0], 0);

    SetAllBitsInRange(words, 5, 1);
    EXPECT_EQ(words[0], 1 << 5);
    SetAllBitsInRange(words, 6, 1);
    EXPECT_EQ(words[0], 3 << 5);
    words[0] = 0;

    SetAllBitsInRange(words, 0, 1);
    EXPECT_EQ(words[0], 1);
    words[0] = 0;

    SetAllBitsInRange(words, kBitSetBitsPerWord - 1, 1);
    EXPECT_EQ(words[0], static_cast<BitSetWord>(1) << (kBitSetBitsPerWord - 1));
    words[0] = 0;

    SetAllBitsInRange(words, 0, kBitSetBitsPerWord);
    EXPECT_EQ(words[0], Constants::kMaxValue<BitSetWord>);
}

TEST(SetBitsInRange, SingleWordWithOffset)
{
    using namespace Internal;

    BitSetWord* memory = AllocateGuardedWords(2);
    auto freeMemory = festd::defer([memory] {
        FreeGuardedWords(memory, 2);
    });

    const festd::span words{ memory, 2 };

    SetAllBitsInRange(words, kBitSetBitsPerWord, 0);
    EXPECT_EQ(words[0], 0);
    EXPECT_EQ(words[1], 0);
    SetAllBitsInRange(words, kBitSetBitsPerWord + 5, 0);
    EXPECT_EQ(words[0], 0);
    EXPECT_EQ(words[1], 0);
    SetAllBitsInRange(words, kBitSetBitsPerWord + kBitSetBitsPerWord, 0);
    EXPECT_EQ(words[0], 0);
    EXPECT_EQ(words[1], 0);

    SetAllBitsInRange(words, kBitSetBitsPerWord + 5, 1);
    EXPECT_EQ(words[0], 0);
    EXPECT_EQ(words[1], 1 << 5);
    SetAllBitsInRange(words, kBitSetBitsPerWord + 6, 1);
    EXPECT_EQ(words[0], 0);
    EXPECT_EQ(words[1], 3 << 5);
    words[0] = 0;
    words[1] = 0;

    SetAllBitsInRange(words, kBitSetBitsPerWord + 0, 1);
    EXPECT_EQ(words[0], 0);
    EXPECT_EQ(words[1], 1);
    words[0] = 0;
    words[1] = 0;

    SetAllBitsInRange(words, kBitSetBitsPerWord + 0, kBitSetBitsPerWord);
    EXPECT_EQ(words[0], 0);
    EXPECT_EQ(words[1], Constants::kMaxValue<BitSetWord>);
}

TEST(SetBitsInRange, AcrossTwoWords)
{
    using namespace Internal;

    BitSetWord* memory = AllocateGuardedWords(2);
    auto freeMemory = festd::defer([memory] {
        FreeGuardedWords(memory, 2);
    });

    const festd::span words{ memory, 2 };

    SetAllBitsInRange(words, kBitSetBitsPerWord - 1, 2);
    EXPECT_EQ(words[0], static_cast<BitSetWord>(1) << (kBitSetBitsPerWord - 1));
    EXPECT_EQ(words[1], 1);
    words[0] = 0;
    words[1] = 0;

    SetAllBitsInRange(words, kBitSetBitsPerWord - 2, 4);
    EXPECT_EQ(words[0], static_cast<BitSetWord>(3) << (kBitSetBitsPerWord - 2));
    EXPECT_EQ(words[1], 3);
}

TEST(SetBitsInRange, AcrossThreeWords)
{
    using namespace Internal;

    BitSetWord* memory = AllocateGuardedWords(3);
    auto freeMemory = festd::defer([memory] {
        FreeGuardedWords(memory, 3);
    });

    const festd::span words{ memory, 3 };

    SetAllBitsInRange(words, kBitSetBitsPerWord - 1, 2 + kBitSetBitsPerWord);
    EXPECT_EQ(words[0], static_cast<BitSetWord>(1) << (kBitSetBitsPerWord - 1));
    EXPECT_EQ(words[1], Constants::kMaxValue<BitSetWord>);
    EXPECT_EQ(words[2], 1);
    words[0] = 0;
    words[1] = 0;
    words[2] = 0;

    SetAllBitsInRange(words, kBitSetBitsPerWord - 2, 4 + kBitSetBitsPerWord);
    EXPECT_EQ(words[0], static_cast<BitSetWord>(3) << (kBitSetBitsPerWord - 2));
    EXPECT_EQ(words[1], Constants::kMaxValue<BitSetWord>);
    EXPECT_EQ(words[2], 3);
}

TEST(SetBitsInRange, AcrossThreeWordsWithOffset)
{
    using namespace Internal;

    BitSetWord* memory = AllocateGuardedWords(4);
    memory[0] = 0xdeadbeefdeadbeef;
    auto freeMemory = festd::defer([memory] {
        FreeGuardedWords(memory, 4);
    });

    const festd::span words{ memory, 4 };

    SetAllBitsInRange(words, 2 * kBitSetBitsPerWord - 1, 2 + kBitSetBitsPerWord);
    EXPECT_EQ(words[0], 0xdeadbeefdeadbeef);
    EXPECT_EQ(words[1], static_cast<BitSetWord>(1) << (kBitSetBitsPerWord - 1));
    EXPECT_EQ(words[2], Constants::kMaxValue<BitSetWord>);
    EXPECT_EQ(words[3], 1);
    words[1] = 0;
    words[2] = 0;
    words[3] = 0;

    SetAllBitsInRange(words, 2 * kBitSetBitsPerWord - 2, 4 + kBitSetBitsPerWord);
    EXPECT_EQ(words[0], 0xdeadbeefdeadbeef);
    EXPECT_EQ(words[1], static_cast<BitSetWord>(3) << (kBitSetBitsPerWord - 2));
    EXPECT_EQ(words[2], Constants::kMaxValue<BitSetWord>);
    EXPECT_EQ(words[3], 3);
}

TEST(ResetBitsInRange, SingleWord)
{
    using namespace Internal;

    BitSetWord* memory = AllocateGuardedWords(1);
    memset(memory, 0xff, sizeof(BitSetWord));

    auto freeMemory = festd::defer([memory] {
        FreeGuardedWords(memory, 1);
    });

    const festd::span words{ memory, 1 };

    ResetAllBitsInRange(words, 0, 0);
    EXPECT_EQ(words[0], Constants::kMaxValue<BitSetWord>);
    ResetAllBitsInRange(words, 5, 0);
    EXPECT_EQ(words[0], Constants::kMaxValue<BitSetWord>);
    ResetAllBitsInRange(words, kBitSetBitsPerWord, 0);
    EXPECT_EQ(words[0], Constants::kMaxValue<BitSetWord>);

    ResetAllBitsInRange(words, 5, 1);
    EXPECT_EQ(words[0], ~static_cast<BitSetWord>(1 << 5));
    ResetAllBitsInRange(words, 6, 1);
    EXPECT_EQ(words[0], ~static_cast<BitSetWord>(3 << 5));
    words[0] = Constants::kMaxValue<BitSetWord>;

    ResetAllBitsInRange(words, 0, 1);
    EXPECT_EQ(words[0], ~static_cast<BitSetWord>(1));
    words[0] = Constants::kMaxValue<BitSetWord>;

    ResetAllBitsInRange(words, kBitSetBitsPerWord - 1, 1);
    EXPECT_EQ(words[0], Constants::kMaxValue<BitSetWord> >> 1);
    words[0] = Constants::kMaxValue<BitSetWord>;

    ResetAllBitsInRange(words, 0, kBitSetBitsPerWord);
    EXPECT_EQ(words[0], 0);
}

TEST(ResetBitsInRange, AcrossThreeWordsWithOffset)
{
    using namespace Internal;

    BitSetWord* memory = AllocateGuardedWords(4);
    memory[0] = 0xdeadbeefdeadbeef;
    memset(memory + 1, 0xff, sizeof(BitSetWord) * 3);

    auto freeMemory = festd::defer([memory] {
        FreeGuardedWords(memory, 4);
    });

    const festd::span words{ memory, 4 };

    ResetAllBitsInRange(words, 2 * kBitSetBitsPerWord - 1, 2 + kBitSetBitsPerWord);
    EXPECT_EQ(words[0], 0xdeadbeefdeadbeef);
    EXPECT_EQ(words[1], ~(static_cast<BitSetWord>(1) << (kBitSetBitsPerWord - 1)));
    EXPECT_EQ(words[2], 0);
    EXPECT_EQ(words[3], ~static_cast<BitSetWord>(1));
    words[1] = Constants::kMaxValue<BitSetWord>;
    words[2] = Constants::kMaxValue<BitSetWord>;
    words[3] = Constants::kMaxValue<BitSetWord>;

    ResetAllBitsInRange(words, 2 * kBitSetBitsPerWord - 2, 4 + kBitSetBitsPerWord);
    EXPECT_EQ(words[0], 0xdeadbeefdeadbeef);
    EXPECT_EQ(words[1], Constants::kMaxValue<BitSetWord> >> 2);
    EXPECT_EQ(words[2], 0);
    EXPECT_EQ(words[3], ~static_cast<BitSetWord>(3));
}

TEST(DynamicBitSet, Basic)
{
    TestAllocator allocator;

    auto memLeakGuard = festd::defer([&allocator] {
        ASSERT_EQ(allocator.m_allocationCount, allocator.m_deallocationCount);
        ASSERT_EQ(allocator.m_totalSize, 0);
    });

    {
        festd::bit_vector bits;
        bits.resize(1, true);
        EXPECT_TRUE(bits.test(0));
        EXPECT_EQ(bits.size(), 1);
    }

    {
        festd::pmr::bit_vector bits{ &allocator };
        bits.resize(1, true);
        EXPECT_TRUE(bits.test(0));
        EXPECT_EQ(bits.size(), 1);
    }
}

TEST(DynamicBitSet, Resize)
{
    TestAllocator allocator;

    auto memLeakGuard = festd::defer([&allocator] {
        ASSERT_EQ(allocator.m_allocationCount, allocator.m_deallocationCount);
        ASSERT_EQ(allocator.m_totalSize, 0);
    });

    const uint32_t chunkSize = 11;
    const uint32_t chunkCount = 300;

    festd::pmr::bit_vector bits{ &allocator };

    for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
    {
        bits.resize(bits.size() + chunkSize, true);
        bits.resize(bits.size() + chunkSize, false);
    }

    for (uint32_t i = 0; i < chunkSize * chunkCount; ++i)
    {
        ASSERT_EQ(bits.test(i), (i / chunkSize) % 2 == 0);
    }
}

TEST(DynamicBitSet, FindFirst)
{
    TestAllocator allocator;

    auto memLeakGuard = festd::defer([&allocator] {
        ASSERT_EQ(allocator.m_allocationCount, allocator.m_deallocationCount);
        ASSERT_EQ(allocator.m_totalSize, 0);
    });

    for (uint32_t bitIndex = 0; bitIndex < 10000; ++bitIndex)
    {
        festd::pmr::bit_vector bits{ &allocator };
        bits.resize(bitIndex + 1, false);
        bits.set(bitIndex);

        ASSERT_EQ(bits.find_first(), bitIndex);
    }
}

TEST(DynamicBitSet, Traverse)
{
    TestAllocator allocator;

    auto memLeakGuard = festd::defer([&allocator] {
        ASSERT_EQ(allocator.m_allocationCount, allocator.m_deallocationCount);
        ASSERT_EQ(allocator.m_totalSize, 0);
    });

    constexpr uint32_t iterationCount = 10000;
    constexpr uint32_t arraySize = 50000;
    constexpr uint32_t maxSetBits = 500;

    std::mt19937 mt(0);
    std::uniform_int_distribution<uint32_t> distIndices(0, arraySize - 1);
    std::uniform_int_distribution<uint32_t> distBitCount(0, maxSetBits);

    for (uint32_t i = 0; i < iterationCount; ++i)
    {
        festd::pmr::bit_vector bits{ &allocator };
        bits.resize(arraySize, false);

        const uint32_t bitsToSet = distBitCount(mt);

        festd::vector<uint32_t> setIndices;
        setIndices.reserve(bitsToSet);

        for (uint32_t bitIndex = 0; bitIndex < bitsToSet; ++bitIndex)
        {
            const uint32_t index = distIndices(mt);

            const auto iter = eastl::lower_bound(setIndices.begin(), setIndices.end(), index);
            if (*iter != index)
                setIndices.insert(iter, index);

            bits.set(index);
        }

        uint32_t currentTraversalIndex = 0;
        Bit::Traverse(bits.view(), [&](uint32_t bitIndex) {
            ASSERT_EQ(setIndices[currentTraversalIndex++], bitIndex);
        });
    }
}

TEST(FixedBitSet, Traverse)
{
    constexpr uint32_t iterationCount = 10000;
    constexpr uint32_t arraySize = 50000;
    constexpr uint32_t maxSetBits = 500;

    std::mt19937 mt(0);
    std::uniform_int_distribution<uint32_t> distIndices(0, arraySize - 1);
    std::uniform_int_distribution<uint32_t> distBitCount(0, maxSetBits);

    for (uint32_t i = 0; i < iterationCount; ++i)
    {
        festd::fixed_bit_vector<arraySize> bits;
        bits.resize(arraySize, false);

        const uint32_t bitsToSet = distBitCount(mt);

        festd::vector<uint32_t> setIndices;
        setIndices.reserve(bitsToSet);

        for (uint32_t bitIndex = 0; bitIndex < bitsToSet; ++bitIndex)
        {
            const uint32_t index = distIndices(mt);

            const auto iter = eastl::lower_bound(setIndices.begin(), setIndices.end(), index);
            if (*iter != index)
                setIndices.insert(iter, index);

            bits.set(index);
        }

        uint32_t currentTraversalIndex = 0;
        Bit::Traverse(bits.view(), [&](uint32_t bitIndex) {
            ASSERT_EQ(setIndices[currentTraversalIndex++], bitIndex);
        });
    }
}

TEST(BitSet, OperatorAnd)
{
    using namespace Internal;

    festd::fixed_bit_vector<100> a;
    festd::fixed_bit_vector<120> b;

    a.resize(kBitSetBitsPerWord + 2, false);
    b.resize(kBitSetBitsPerWord + 2, false);

    a.set(0);
    a.set(1);
    a.set(kBitSetBitsPerWord - 1);
    a.set(kBitSetBitsPerWord);

    b.set(0);
    b.set(kBitSetBitsPerWord + 1);

    Bit::Traverse(a.view() & b.view(), [&](uint32_t bitIndex) {
        ASSERT_EQ(bitIndex, 0);
    });
}

TEST(BitSet, OperatorOr)
{
    using namespace Internal;

    festd::fixed_bit_vector<100> a;
    festd::fixed_bit_vector<120> b;

    a.resize(kBitSetBitsPerWord + 2, false);
    b.resize(kBitSetBitsPerWord + 2, false);

    a.set(kBitSetBitsPerWord - 1);
    a.set(kBitSetBitsPerWord);

    b.set(kBitSetBitsPerWord + 1);

    uint32_t currentTraversalIndex = 0;
    Bit::Traverse(a.view() & b.view(), [&](uint32_t bitIndex) {
        ASSERT_EQ(bitIndex, kBitSetBitsPerWord - 1 + currentTraversalIndex++);
    });
}
