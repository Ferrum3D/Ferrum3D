#pragma once
#include <FeCore/Logging/Trace.h>
#include <FeCore/Memory/Memory.h>

namespace FE
{
    using BitSetWord = uint64_t;
}


namespace FE::Internal
{
    inline constexpr uint32_t kBitSetWordBitShift = 6;
    inline constexpr uint32_t kBitSetBitsPerWord = sizeof(BitSetWord) * 8;
    static_assert(1 << kBitSetWordBitShift == kBitSetBitsPerWord);


    constexpr uint32_t GetNewBitSetCapacity(uint32_t currentCapacity, uint32_t minimum)
    {
        return Math::Max(minimum, currentCapacity * 2);
    }


    constexpr uint32_t CalculateNextLevelWordCount(uint32_t count)
    {
        return (count + kBitSetBitsPerWord - 1) >> kBitSetWordBitShift;
    }


    constexpr uint32_t CalculateWordIndex(uint32_t bitIndex)
    {
        return bitIndex >> kBitSetWordBitShift;
    }


    struct DynamicBitSetStorage
    {
        uint32_t m_size = 0;
        uint32_t m_capacity = 0;
        BitSetWord* m_words = nullptr;
        BitSetWord* m_topLevel = nullptr;

        [[nodiscard]] uint32_t SizeImpl() const
        {
            return m_size;
        }

        [[nodiscard]] uint32_t CapacityImpl() const
        {
            return m_capacity;
        }

        [[nodiscard]] BitSetWord* WordsDataImpl()
        {
            return m_words;
        }

        [[nodiscard]] const BitSetWord* WordsDataImpl() const
        {
            return m_words;
        }

        [[nodiscard]] BitSetWord* TopLevelDataImpl()
        {
            return m_topLevel;
        }

        [[nodiscard]] const BitSetWord* TopLevelDataImpl() const
        {
            return m_topLevel;
        }

        template<bool TKeepData>
        FE_FORCE_INLINE void GrowImpl(uint32_t length, uint32_t capacity, std::pmr::memory_resource* allocator)
        {
            //
            // While some parts of the top-level mask of a hierarchical bitset can just be set to zeros and
            // do not correspond to any of the bits in the storage, it is impossible to store a number
            // of actual storage bits that is not aligned to 64.
            //

            capacity = AlignUp(capacity, sizeof(BitSetWord) * 8);

            const uint32_t wordCount = CalculateNextLevelWordCount(capacity);
            const uint32_t topLevelWordCount = CalculateNextLevelWordCount(wordCount);
            const uint32_t totalAllocatedWordCount = wordCount + topLevelWordCount;

            BitSetWord* newWords = Memory::AllocateArray<BitSetWord>(allocator, totalAllocatedWordCount);
            BitSetWord* newTopLevel = newWords + wordCount;

            const uint32_t usedWordCount = CalculateNextLevelWordCount(length);
            const uint32_t usedTopLevelWordCount = CalculateNextLevelWordCount(usedWordCount);

            if (length > 0)
            {
                //
                // We have to ensure that the last used words do not have any garbage set bits
                // in the inaccessible part since the traversal algorithm doesn't perform any bound checks.
                // It is important to zero these words prior to any copy operations to avoid overwriting the data.
                //

                newWords[usedWordCount - 1] = 0;
                newTopLevel[usedTopLevelWordCount - 1] = 0;
            }

            if constexpr (TKeepData)
            {
                const uint32_t copyWordCount = CalculateNextLevelWordCount(m_size);
                const uint32_t copyTopLevelWordCount = CalculateNextLevelWordCount(copyWordCount);
                memcpy(newWords, m_words, copyWordCount * sizeof(BitSetWord));
                memcpy(newTopLevel, m_topLevel, copyTopLevelWordCount * sizeof(BitSetWord));
            }

            if (m_words != nullptr)
            {
                const uint32_t oldWordCount = CalculateNextLevelWordCount(m_capacity);
                const uint32_t oldTopLevelWordCount = CalculateNextLevelWordCount(oldWordCount);
                const uint32_t oldTotalAllocatedWordCount = oldWordCount + oldTopLevelWordCount;
                allocator->deallocate(m_words, oldTotalAllocatedWordCount * sizeof(BitSetWord));
            }

            m_words = newWords;
            m_topLevel = newTopLevel;

            m_size = length;
            m_capacity = capacity;
            FE_AssertDebug(length <= capacity);
        }

        void InitializeImpl(uint32_t length, std::pmr::memory_resource* allocator)
        {
            GrowImpl<false>(length, length, allocator);
        }

        void ReinitializeImpl(uint32_t length, std::pmr::memory_resource* allocator)
        {
            GrowImpl<false>(length, length, allocator);
        }

        void ReserveImpl(uint32_t length, std::pmr::memory_resource* allocator)
        {
            if (length > m_capacity)
                GrowImpl<true>(m_size, length, allocator);
        }

        void ResizeImpl(uint32_t length, std::pmr::memory_resource* allocator)
        {
            GrowImpl<true>(length, Math::Max(m_capacity, length), allocator);
        }

        void ShrinkImpl(std::pmr::memory_resource* allocator)
        {
            const uint32_t minWordCount = CalculateNextLevelWordCount(m_size);
            const uint32_t minCapacity = minWordCount * sizeof(BitSetWord) * 8;
            if (minCapacity < m_capacity)
                GrowImpl<true>(m_size, minCapacity, allocator);
        }

        void DestroyImpl(std::pmr::memory_resource* allocator)
        {
            if (m_words != nullptr)
            {
                const uint32_t wordCount = CalculateNextLevelWordCount(m_capacity);
                const uint32_t topLevelWordCount = CalculateNextLevelWordCount(wordCount);
                allocator->deallocate(m_words, (wordCount + topLevelWordCount) * sizeof(BitSetWord));
            }

            m_words = nullptr;
            m_topLevel = nullptr;
        }
    };


    template<uint32_t TMinCapacity>
    struct FixedBitSetStorage
    {
        static constexpr uint32_t kCapacity = AlignUp(TMinCapacity, sizeof(BitSetWord) * 8);
        static constexpr uint32_t kWordCount = CalculateNextLevelWordCount(kCapacity);
        static constexpr uint32_t kTopLevelWordCount = CalculateNextLevelWordCount(kWordCount);

        uint32_t m_size = 0;
        BitSetWord m_words[kWordCount];
        BitSetWord m_topLevel[kTopLevelWordCount];

        [[nodiscard]] uint32_t SizeImpl() const
        {
            return m_size;
        }

        [[nodiscard]] uint32_t CapacityImpl() const
        {
            return kCapacity;
        }

        [[nodiscard]] BitSetWord* WordsDataImpl()
        {
            return m_words;
        }

        [[nodiscard]] const BitSetWord* WordsDataImpl() const
        {
            return m_words;
        }

        [[nodiscard]] BitSetWord* TopLevelDataImpl()
        {
            return m_topLevel;
        }

        [[nodiscard]] const BitSetWord* TopLevelDataImpl() const
        {
            return m_topLevel;
        }

        void InitializeImpl(uint32_t length, std::pmr::memory_resource*)
        {
            FE_AssertDebug(length <= kCapacity);
            memset(this, 0, sizeof(*this));
            m_size = length;
        }

        void ReinitializeImpl(uint32_t length, std::pmr::memory_resource*)
        {
            FE_AssertDebug(length <= kCapacity);
            m_size = length;
        }

        void ReserveImpl(uint32_t length, std::pmr::memory_resource*)
        {
            FE_AssertDebug(length <= kCapacity);
        }

        void ResizeImpl(uint32_t length, std::pmr::memory_resource*)
        {
            FE_AssertDebug(length <= kCapacity);
            m_size = length;
        }

        void ShrinkImpl(std::pmr::memory_resource*) {}

        void DestroyImpl(std::pmr::memory_resource*) {}
    };
} // namespace FE::Internal
