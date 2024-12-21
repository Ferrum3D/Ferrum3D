#pragma once
#include <festd/Internal/BitStorage.h>

namespace FE::Internal
{
    inline void SetAllBitsInRange(festd::span<BitSetWord> words, uint32_t startBitIndex, uint32_t bitCount)
    {
        if (bitCount == 0)
            return;

        const uint32_t startWordIndex = CalculateWordIndex(startBitIndex);
        const uint32_t startBitIndexInWord = startBitIndex - startWordIndex * kBitSetBitsPerWord;

        if (startBitIndexInWord + bitCount <= kBitSetBitsPerWord)
        {
            // Special case: set bits in a single word
            words[startWordIndex] |= MakeMask<BitSetWord>(bitCount, startBitIndexInWord);
        }
        else
        {
            const uint32_t endWordIndex = CalculateWordIndex(startBitIndex + bitCount - 1);
            const uint32_t endBitIndexInWord = startBitIndex + bitCount - endWordIndex * kBitSetBitsPerWord;
            words[startWordIndex] |= std::numeric_limits<BitSetWord>::max() << startBitIndexInWord;
            words[endWordIndex] |= MakeMask<BitSetWord>(endBitIndexInWord, 0);

            if (endWordIndex > startWordIndex + 1)
            {
                const size_t wordCountInBetween = endWordIndex - startWordIndex - 1;
                memset(words.data() + startWordIndex + 1, 0xff, sizeof(BitSetWord) * wordCountInBetween);
            }
        }
    }


    inline void ResetAllBitsInRange(festd::span<BitSetWord> words, uint32_t startBitIndex, uint32_t bitCount)
    {
        if (bitCount == 0)
            return;

        const uint32_t startWordIndex = CalculateWordIndex(startBitIndex);
        const uint32_t startBitIndexInWord = startBitIndex - startWordIndex * kBitSetBitsPerWord;

        if (startBitIndexInWord + bitCount <= kBitSetBitsPerWord)
        {
            // Special case: reset bits in a single word
            words[startWordIndex] &= ~MakeMask<BitSetWord>(bitCount, startBitIndexInWord);
        }
        else
        {
            const uint32_t endWordIndex = CalculateWordIndex(startBitIndex + bitCount - 1);
            const uint32_t endBitIndexInWord = startBitIndex + bitCount - endWordIndex * kBitSetBitsPerWord;
            words[startWordIndex] &= ~(std::numeric_limits<BitSetWord>::max() << startBitIndexInWord);
            words[endWordIndex] &= ~MakeMask<BitSetWord>(endBitIndexInWord, 0);

            if (endWordIndex > startWordIndex + 1)
            {
                const size_t wordCountInBetween = endWordIndex - startWordIndex - 1;
                memset(words.data() + startWordIndex + 1, 0x0, sizeof(BitSetWord) * wordCountInBetween);
            }
        }
    }


    template<class TStorage>
    struct DefaultAllocatorBitSetStorage : public TStorage
    {
        static constexpr bool kHasAllocator = false;

        [[nodiscard]] std::pmr::memory_resource* GetAllocator() const
        {
            return std::pmr::get_default_resource();
        }

        void SetAllocator(std::pmr::memory_resource*) {}
    };


    template<class TStorage>
    struct PolymorphicAllocatorBitSetStorage : public TStorage
    {
        static constexpr bool kHasAllocator = true;

        std::pmr::memory_resource* m_allocator;

        PolymorphicAllocatorBitSetStorage()
        {
            m_allocator = std::pmr::get_default_resource();
        }

        explicit PolymorphicAllocatorBitSetStorage(std::pmr::memory_resource* allocator)
            : m_allocator(allocator)
        {
            if (allocator == nullptr)
                m_allocator = std::pmr::get_default_resource();
        }

        void SetAllocator(std::pmr::memory_resource* allocator)
        {
            m_allocator = allocator;
        }

        [[nodiscard]] std::pmr::memory_resource* GetAllocator() const
        {
            return m_allocator;
        }
    };


    template<class TView>
    struct BasicBitSetView : public TView
    {
        using TView::TView;
    };


    template<class TLeftView, class TRightView>
    struct BitSetOperatorOr
    {
        BitSetOperatorOr(const BasicBitSetView<TLeftView>& lhs, const BasicBitSetView<TRightView>& rhs)
            : m_leftView(lhs)
            , m_rightView(rhs)
        {
        }

        [[nodiscard]] BitSetWord data_at(uint32_t wordIndex) const
        {
            return m_leftView.data_at(wordIndex) | m_rightView.data_at(wordIndex);
        }

        [[nodiscard]] BitSetWord data_lookup_at(uint32_t wordIndex) const
        {
            return m_leftView.data_lookup_at(wordIndex) | m_rightView.data_lookup_at(wordIndex);
        }

        [[nodiscard]] uint32_t size() const
        {
            return m_leftView.size();
        }

        BasicBitSetView<TLeftView> m_leftView;
        BasicBitSetView<TRightView> m_rightView;
    };


    template<class TLeftView, class TRightView>
    struct BitSetOperatorAnd
    {
        BitSetOperatorAnd(const BasicBitSetView<TLeftView>& lhs, const BasicBitSetView<TRightView>& rhs)
            : m_leftView(lhs)
            , m_rightView(rhs)
        {
        }

        [[nodiscard]] BitSetWord data_at(uint32_t wordIndex) const
        {
            return m_leftView.data_at(wordIndex) & m_rightView.data_at(wordIndex);
        }

        [[nodiscard]] BitSetWord data_lookup_at(uint32_t wordIndex) const
        {
            // This can give a false-positive result, e.g.
            // if the left data word is 0x1010 and the right one is 0x0101:
            // both lookup bits are ones but the actual word calculated as a bitwise AND is zero.
            // It's okay, but we have to consider such possibility when we implement our traversal algorithms.
            return m_leftView.data_lookup_at(wordIndex) & m_rightView.data_lookup_at(wordIndex);
        }

        [[nodiscard]] uint32_t size() const
        {
            return m_leftView.size();
        }

        BasicBitSetView<TLeftView> m_leftView;
        BasicBitSetView<TRightView> m_rightView;
    };


    struct BasicBitSetViewImpl
    {
        BasicBitSetViewImpl() = default;

        BasicBitSetViewImpl(const uint64_t* words, const uint64_t* topLevelWords, uint32_t bitCount)
            : m_words(words)
            , m_topLevel(topLevelWords)
            , m_size(bitCount)
        {
        }

        [[nodiscard]] BitSetWord data_at(uint32_t wordIndex) const
        {
            return m_words[wordIndex];
        }

        [[nodiscard]] BitSetWord data_lookup_at(uint32_t wordIndex) const
        {
            return m_topLevel[wordIndex];
        }

        [[nodiscard]] uint32_t size() const
        {
            return m_size;
        }

    private:
        const BitSetWord* m_words = nullptr;
        const BitSetWord* m_topLevel = nullptr;
        uint32_t m_size = 0;
    };


    template<class TStorage>
    struct BasicBitSetImpl : private TStorage
    {
        BasicBitSetImpl()
        {
            TStorage::InitializeImpl(0, TStorage::GetAllocator());
        }

        template<class = std::enable_if_t<TStorage::kHasAllocator>>
        explicit BasicBitSetImpl(std::pmr::memory_resource* allocator)
            : TStorage(allocator)
        {
        }

        ~BasicBitSetImpl()
        {
            TStorage::DestroyImpl(TStorage::GetAllocator());
        }

        BasicBitSetImpl(const BasicBitSetImpl& other)
        {
            const uint32_t size = other.SizeImpl();
            TStorage::InitializeImpl(size, other.GetAllocator());
            const BitSetWord* words = other.WordsDataImpl();
            const BitSetWord* topLevelWords = other.TopLevelDataImpl();

            const uint32_t wordCount = CalculateNextLevelWordCount(size);
            const uint32_t topLevelWordCount = CalculateNextLevelWordCount(wordCount);

            memcpy(TStorage::WordsDataImpl(), words, wordCount * sizeof(BitSetWord));
            memcpy(TStorage::TopLevelDataImpl(), topLevelWords, topLevelWordCount * sizeof(BitSetWord));
        }

        BasicBitSetImpl(BasicBitSetImpl&& other) noexcept
        {
            memcpy(this, &other, sizeof(*this)); // NOLINT
            other.InitializeImpl(0, other.GetAllocator());
        }

        BasicBitSetImpl& operator=(const BasicBitSetImpl& other) noexcept
        {
            const uint32_t size = other.SizeImpl();

            if (TStorage::kHasAllocator && other.GetAllocator() != TStorage::GetAllocator())
            {
                DestroyImpl(TStorage::GetAllocator());
                TStorage::InitializeImpl(size, other.GetAllocator());
            }
            else
            {
                TStorage::ReinitializeImpl(size, TStorage::GetAllocator());
            }

            const BitSetWord* words = other.WordsDataImpl();
            const BitSetWord* topLevelWords = other.TopLevelDataImpl();

            const uint32_t wordCount = CalculateNextLevelWordCount(size);
            const uint32_t topLevelWordCount = CalculateNextLevelWordCount(wordCount);

            memcpy(TStorage::WordsDataImpl(), words, wordCount * sizeof(BitSetWord));
            memcpy(TStorage::TopLevelDataImpl(), topLevelWords, topLevelWordCount * sizeof(BitSetWord));
            return *this;
        }

        BasicBitSetImpl& operator=(BasicBitSetImpl&& other) noexcept
        {
            TStorage::DestroyImpl(TStorage::GetAllocator());
            memcpy(this, &other, sizeof(*this)); // NOLINT
            other.InitializeImpl(0, other.GetAllocator());
            return *this;
        }

        void set()
        {
            BitSetWord* words = TStorage::WordsDataImpl();
            BitSetWord* topLevelWords = TStorage::TopLevelDataImpl();

            const uint32_t wordCount = CalculateNextLevelWordCount(TStorage::SizeImpl());
            const uint32_t topLevelWordCount = CalculateNextLevelWordCount(wordCount);

            SetAllBitsInRange({ words, wordCount }, 0, TStorage::SizeImpl());
            SetAllBitsInRange({ topLevelWords, topLevelWordCount }, 0, wordCount);
        }

        void reset()
        {
            BitSetWord* words = TStorage::WordsDataImpl();
            BitSetWord* topLevelWords = TStorage::TopLevelDataImpl();

            const uint32_t wordCount = CalculateNextLevelWordCount(TStorage::SizeImpl());
            const uint32_t topLevelWordCount = CalculateNextLevelWordCount(wordCount);

            ResetAllBitsInRange({ words, wordCount }, 0, TStorage::SizeImpl());
            ResetAllBitsInRange({ topLevelWords, topLevelWordCount }, 0, wordCount);
        }

        void set(uint32_t bitIndex)
        {
            BitSetWord* words = TStorage::WordsDataImpl();
            const uint32_t wordIndex = CalculateWordIndex(bitIndex);

            words[wordIndex] |= UINT64_C(1) << (bitIndex - wordIndex * kBitSetBitsPerWord);
            SetTopLevel(wordIndex);
        }

        void reset(uint32_t bitIndex)
        {
            BitSetWord* words = TStorage::WordsDataImpl();
            const uint32_t wordIndex = CalculateWordIndex(bitIndex);

            words[wordIndex] |= UINT64_C(1) << (bitIndex - wordIndex * kBitSetBitsPerWord);
            if (words[wordIndex] == 0)
                ResetTopLevel(wordIndex);
        }


        void set(uint32_t bitIndex, bool value)
        {
            if (value)
                set(bitIndex);
            else
                reset(bitIndex);
        }

        void reserve(uint32_t bitCount)
        {
            TStorage::ReserveImpl(bitCount, TStorage::GetAllocator());
        }

        void resize(uint32_t bitCount, bool bitValue)
        {
            const uint32_t prevSize = TStorage::SizeImpl();

            TStorage::ResizeImpl(bitCount, TStorage::GetAllocator());

            const uint32_t wordCount = CalculateNextLevelWordCount(TStorage::SizeImpl());

            BitSetWord* words = TStorage::WordsDataImpl();

            if (bitValue)
            {
                SetAllBitsInRange({ words, wordCount }, prevSize, bitCount - prevSize);
            }
            else
            {
                ResetAllBitsInRange({ words, wordCount }, prevSize, bitCount - prevSize);
            }

            // TODO: optimize this loop: calculate the values of the first and the last top-level words
            //       and use memset in between.
            for (uint32_t wordIndex = CalculateWordIndex(prevSize); wordIndex < wordCount; ++wordIndex)
            {
                if (words[wordIndex] == 0)
                    ResetTopLevel(wordIndex);
                else
                    SetTopLevel(wordIndex);
            }
        }

        void shrink_to_fit()
        {
            TStorage::ShrinkImpl(TStorage::GetAllocator());
        }

        [[nodiscard]] std::pmr::memory_resource* get_allocator() const
        {
            return TStorage::GetAllocator();
        }

        template<class = std::enable_if_t<TStorage::kHasAllocator>>
        void set_allocator(std::pmr::memory_resource* allocator)
        {
            FE_AssertDebug(TStorage::SizeImpl() == 0);
            TStorage::SetAllocator(allocator);
        }

        [[nodiscard]] uint32_t size() const
        {
            return TStorage::SizeImpl();
        }

        [[nodiscard]] bool empty() const
        {
            return TStorage::SizeImpl() == 0;
        }

        [[nodiscard]] uint32_t capacity() const
        {
            return TStorage::CapacityImpl();
        }

        [[nodiscard]] BitSetWord* data()
        {
            return TStorage::WordsDataImpl();
        }

        [[nodiscard]] const BitSetWord* data() const
        {
            return TStorage::WordsDataImpl();
        }

        [[nodiscard]] BitSetWord* data_lookup()
        {
            return TStorage::TopLevelDataImpl();
        }

        [[nodiscard]] const BitSetWord* data_lookup() const
        {
            return TStorage::TopLevelDataImpl();
        }

    private:
        FE_FORCE_INLINE void SetTopLevel(uint32_t bitIndex)
        {
            BitSetWord* topLevelWords = TStorage::TopLevelDataImpl();
            const uint32_t wordIndex = CalculateWordIndex(bitIndex);

            topLevelWords[wordIndex] |= UINT64_C(1) << (bitIndex - wordIndex * kBitSetBitsPerWord);
        }

        FE_FORCE_INLINE void ResetTopLevel(uint32_t bitIndex)
        {
            BitSetWord* topLevelWords = TStorage::TopLevelDataImpl();
            const uint32_t wordIndex = CalculateWordIndex(bitIndex);

            topLevelWords[wordIndex] &= ~(UINT64_C(1) << (bitIndex - wordIndex * kBitSetBitsPerWord));
        }
    };


    template<class TBase>
    struct BitSetImpl final : public TBase
    {
        using TBase::TBase;

        [[nodiscard]] bool test(uint32_t bitIndex) const
        {
            const BitSetWord* words = TBase::data();
            const uint32_t wordIndex = CalculateWordIndex(bitIndex);
            return words[wordIndex] & (UINT64_C(1) << (bitIndex - wordIndex * kBitSetBitsPerWord));
        }

        [[nodiscard]] uint32_t find_first() const
        {
            const BitSetWord* words = TBase::data();
            const BitSetWord* topLevelWords = TBase::data_lookup();

            const uint32_t wordCount = CalculateNextLevelWordCount(TBase::size());
            const uint32_t topLevelWordCount = CalculateNextLevelWordCount(wordCount);

            for (uint32_t topLevelIndex = 0; topLevelIndex < topLevelWordCount; ++topLevelIndex)
            {
                uint32_t nonEmptyWordIndex;
                if (Bit::ScanForward64(nonEmptyWordIndex, topLevelWords[topLevelIndex]))
                {
                    const uint32_t wordIndex = topLevelIndex * kBitSetBitsPerWord + nonEmptyWordIndex;

                    uint32_t result;
                    Bit::ScanForward64(result, words[wordIndex]);
                    return result + wordIndex * kBitSetBitsPerWord;
                }
            }

            return kInvalidIndex;
        }

        BitSetImpl<BasicBitSetView<BasicBitSetViewImpl>> view() const
        {
            const BitSetWord* words = TBase::data();
            const BitSetWord* topLevelWords = TBase::data_lookup();
            const uint32_t bitCount = TBase::size();
            return BitSetImpl<BasicBitSetView<BasicBitSetViewImpl>>(words, topLevelWords, bitCount);
        }
    };


    template<class TLeftView, class TRightView>
    auto operator&(const BasicBitSetView<TLeftView>& lhs, const BasicBitSetView<TRightView>& rhs)
    {
        return BitSetImpl<BasicBitSetView<BitSetOperatorAnd<TLeftView, TRightView>>>{ lhs, rhs };
    }

    template<class TLeftView, class TRightView>
    auto operator|(const BasicBitSetView<TLeftView>& lhs, const BasicBitSetView<TRightView>& rhs)
    {
        return BitSetImpl<BasicBitSetView<BitSetOperatorOr<TLeftView, TRightView>>>{ lhs, rhs };
    }
} // namespace FE::Internal


namespace FE
{
    namespace festd
    {
        namespace pmr
        {
            using bit_vector = FE::Internal::BitSetImpl<FE::Internal::BasicBitSetImpl<
                FE::Internal::PolymorphicAllocatorBitSetStorage<FE::Internal::DynamicBitSetStorage>>>;
        }

        using bit_vector = FE::Internal::BitSetImpl<
            FE::Internal::BasicBitSetImpl<FE::Internal::DefaultAllocatorBitSetStorage<FE::Internal::DynamicBitSetStorage>>>;

        template<uint32_t TMinCapacity>
        using fixed_bit_vector = FE::Internal::BitSetImpl<FE::Internal::BasicBitSetImpl<
            FE::Internal::DefaultAllocatorBitSetStorage<FE::Internal::FixedBitSetStorage<TMinCapacity>>>>;

        using bit_vector_view = FE::Internal::BitSetImpl<FE::Internal::BasicBitSetView<FE::Internal::BasicBitSetViewImpl>>;
    } // namespace festd


    namespace Bit
    {
        template<class TBase, class TFunctor>
        inline void Traverse(const Internal::BitSetImpl<Internal::BasicBitSetView<TBase>>& bits, TFunctor functor)
        {
            const uint32_t bitCount = bits.size();
            const uint32_t wordCount = Internal::CalculateNextLevelWordCount(bitCount);
            const uint32_t topLevelWordCount = Internal::CalculateNextLevelWordCount(wordCount);
            for (uint32_t topLevelIndex = 0; topLevelIndex < topLevelWordCount; ++topLevelIndex)
            {
                uint32_t nonEmptyWordIndex;
                uint64_t currentLookupWord = bits.data_lookup_at(topLevelIndex);
                while (ScanForward64(nonEmptyWordIndex, currentLookupWord))
                {
                    const uint32_t wordIndex = topLevelIndex * Internal::kBitSetBitsPerWord + nonEmptyWordIndex;

                    uint32_t currentIndex;
                    uint64_t currentWord = bits.data_at(wordIndex);
                    while (ScanForward64(currentIndex, currentWord))
                    {
                        functor(currentIndex + wordIndex * Internal::kBitSetBitsPerWord);
                        currentWord &= ~(UINT64_C(1) << currentIndex);
                    }

                    currentLookupWord &= ~(UINT64_C(1) << nonEmptyWordIndex);
                }
            }
        }
    } // namespace Bit
} // namespace FE
