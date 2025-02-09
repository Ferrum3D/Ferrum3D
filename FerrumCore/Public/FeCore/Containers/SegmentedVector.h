#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE
{
    template<class T, size_t TSegmentByteSize = 4096>
    class SegmentedVector final
    {
        template<class TOther, size_t TSizeOther>
        friend class SegmentedVector;

        static constexpr uint32_t kElementsPerSegment = TSegmentByteSize / sizeof(T);

        static_assert(sizeof(T) <= TSegmentByteSize);

        std::pmr::memory_resource* m_allocator = nullptr;
        T** m_segments = nullptr;
        uint32_t m_segmentTableSize = 0;
        uint32_t m_size = 0;

        void GrowSegmentTable()
        {
            const uint32_t newSize = m_segmentTableSize == 0 ? 1 : m_segmentTableSize * 2;
            T** newTable = Memory::AllocateArray<T*>(m_allocator, newSize);
            memset(static_cast<void*>(newTable + m_segmentTableSize),
                   0,
                   static_cast<size_t>(newSize - m_segmentTableSize) * sizeof(T*));

            if (m_segments)
            {
                memcpy(static_cast<void*>(newTable), static_cast<void*>(m_segments), m_segmentTableSize * sizeof(T*));
                m_allocator->deallocate(static_cast<void*>(m_segments), sizeof(T*) * m_segmentTableSize);
            }

            m_segmentTableSize = newSize;
            m_segments = newTable;
        }

        void ResetImpl()
        {
            if (!m_segments)
                return;

            clear();

            for (uint32_t segmentIndex = 0; segmentIndex < m_segmentTableSize; ++segmentIndex)
            {
                if (m_segments[segmentIndex] == nullptr)
                    break;

                m_allocator->deallocate(m_segments[segmentIndex], TSegmentByteSize, alignof(T));
            }

            m_allocator->deallocate(m_segments, sizeof(T*) * m_segmentTableSize, alignof(T*));
            m_segments = nullptr;
        }

    public:
        template<bool TConst>
        class Iterator final
        {
            friend class SegmentedVector;

            T** m_segments = nullptr;
            uint32_t m_segmentIndex = 0;
            uint32_t m_elementIndex = 0;

            Iterator(T** segments, const uint32_t segmentIndex, const uint32_t elementIndex)
                : m_segments(segments)
                , m_segmentIndex(segmentIndex)
                , m_elementIndex(elementIndex)
            {
            }

        public:
            using difference_type = int32_t;
            using value_type = T;
            using pointer = std::conditional_t<TConst, const T*, T*>;
            using reference = std::conditional_t<TConst, const T&, T&>;

            reference operator*() const
            {
                return m_segments[m_segmentIndex][m_elementIndex];
            }

            pointer operator->() const
            {
                return &m_segments[m_segmentIndex][m_elementIndex];
            }

            Iterator& operator++()
            {
                if (m_elementIndex < kElementsPerSegment - 1)
                {
                    ++m_elementIndex;
                }
                else
                {
                    ++m_segmentIndex;
                    m_elementIndex = 0;
                }

                return *this;
            }

            Iterator operator++(int)
            {
                Iterator t = *this;
                ++(*this);
                return t;
            }

            Iterator& operator--()
            {
                if (m_elementIndex > 0)
                {
                    --m_elementIndex;
                }
                else
                {
                    FE_CORE_ASSERT(m_segmentIndex > 0, "Iterator out of range");
                    --m_segmentIndex;
                    m_elementIndex = kElementsPerSegment - 1;
                }

                return *this;
            }

            Iterator operator--(int)
            {
                Iterator t = *this;
                --(*this);
                return t;
            }

            Iterator operator+(const int32_t rhs) const
            {
                const uint32_t initialIndex = m_segmentIndex * kElementsPerSegment + m_elementIndex;
                const uint32_t newIndex = initialIndex + rhs;
                return Iterator{ m_segments, newIndex / kElementsPerSegment, newIndex % kElementsPerSegment };
            }

            Iterator operator-(const int32_t rhs) const
            {
                const uint32_t initialIndex = m_segmentIndex * kElementsPerSegment + m_elementIndex;
                const uint32_t newIndex = initialIndex - rhs;
                return Iterator{ m_segments, newIndex / kElementsPerSegment, newIndex % kElementsPerSegment };
            }

            Iterator& operator+=(const int32_t rhs)
            {
                const uint32_t initialIndex = m_segmentIndex * kElementsPerSegment + m_elementIndex;
                const uint32_t newIndex = initialIndex + rhs;
                m_segmentIndex = newIndex / kElementsPerSegment;
                m_elementIndex = newIndex % kElementsPerSegment;
                return *this;
            }

            Iterator& operator-=(const int32_t rhs)
            {
                const uint32_t initialIndex = m_segmentIndex * kElementsPerSegment + m_elementIndex;
                const uint32_t newIndex = initialIndex - rhs;
                m_segmentIndex = newIndex / kElementsPerSegment;
                m_elementIndex = newIndex % kElementsPerSegment;
                return *this;
            }

            Iterator operator-(const Iterator& other) const
            {
                const uint32_t lhsIndex = m_segmentIndex * kElementsPerSegment + m_elementIndex;
                const uint32_t rhsIndex = other.m_segmentIndex * kElementsPerSegment + other.m_elementIndex;
                return static_cast<int32_t>(lhsIndex) - static_cast<int32_t>(rhsIndex);
            }

            friend bool operator==(const Iterator& lhs, const Iterator& rhs)
            {
                return lhs.m_segmentIndex == rhs.m_segmentIndex && lhs.m_elementIndex == rhs.m_elementIndex;
            }

            friend bool operator!=(const Iterator& lhs, const Iterator& rhs)
            {
                return !(lhs == rhs);
            }
        };

        SegmentedVector(std::pmr::memory_resource* pAllocator = nullptr)
            : m_allocator(pAllocator)
        {
            if (m_allocator == nullptr)
                m_allocator = std::pmr::get_default_resource();
        }

        ~SegmentedVector()
        {
            ResetImpl();
        }

        SegmentedVector(const SegmentedVector& other)
            : m_allocator(other.m_allocator)
        {
            const uint32_t segmentCount = Math::CeilDivide(other.m_size, kElementsPerSegment);
            m_segmentTableSize = segmentCount;
            m_segments = Memory::AllocateArray<T*>(m_allocator, segmentCount);
            memset(m_segments, 0, sizeof(T*) * segmentCount);
            for (const T& element : other)
            {
                push_back(element);
            }
        }

        template<size_t TSizeOther>
        SegmentedVector(const SegmentedVector<T, TSizeOther>& other)
            : m_allocator(other.m_allocator)
        {
            const uint32_t segmentCount = Math::CeilDivide(other.m_size, kElementsPerSegment);
            m_segmentTableSize = segmentCount;
            m_segments = Memory::AllocateArray<T*>(m_allocator, segmentCount);
            memset(m_segments, 0, sizeof(T*) * segmentCount);
            for (const T& element : other)
            {
                push_back(element);
            }
        }

        SegmentedVector& operator=(const SegmentedVector& other)
        {
            ResetImpl();
            new (this) SegmentedVector(other);
            return *this;
        }

        template<size_t TSizeOther>
        SegmentedVector& operator=(const SegmentedVector<T, TSizeOther>& other)
        {
            ResetImpl();
            new (this) SegmentedVector(other);
            return *this;
        }

        SegmentedVector(SegmentedVector&& other) noexcept
            : m_allocator(other.m_allocator)
            , m_segments(other.m_segments)
            , m_segmentTableSize(other.m_segmentTableSize)
            , m_size(other.m_size)
        {
            other.m_segments = nullptr;
            other.m_size = 0;
            other.m_segmentTableSize = 0;
        }

        SegmentedVector& operator=(SegmentedVector&& other) noexcept
        {
            ResetImpl();
            new (this) SegmentedVector(std::move(other));
            return *this;
        }

        [[nodiscard]] T& operator[](const uint32_t index)
        {
            FE_CORE_ASSERT(index < m_size, "Index out of range");

            const uint32_t segmentIndex = index / kElementsPerSegment;
            const uint32_t elementIndex = index % kElementsPerSegment;
            return m_segments[segmentIndex][elementIndex];
        }

        [[nodiscard]] const T& operator[](const uint32_t index) const
        {
            FE_CORE_ASSERT(index < m_size, "Index out of range");

            const uint32_t segmentIndex = index / kElementsPerSegment;
            const uint32_t elementIndex = index % kElementsPerSegment;
            return m_segments[segmentIndex][elementIndex];
        }

        void* push_back_uninitialized()
        {
            const uint32_t segmentIndex = m_size / kElementsPerSegment;
            const uint32_t elementIndex = m_size % kElementsPerSegment;

            if (m_segmentTableSize <= segmentIndex)
                GrowSegmentTable();

            if (elementIndex == 0)
            {
                if (m_segments[segmentIndex] == nullptr)
                    m_segments[segmentIndex] = static_cast<T*>(m_allocator->allocate(TSegmentByteSize, alignof(T)));
            }
            else
            {
                FE_CORE_ASSERT(m_segments[segmentIndex], "");
            }

            ++m_size;
            return &m_segments[segmentIndex][elementIndex];
        }

        T& push_back()
        {
            T* result = new (push_back_uninitialized()) T{};
            return *result;
        }

        void push_back(const T& value)
        {
            new (push_back_uninitialized()) T(value);
        }

        void push_back(T&& value)
        {
            new (push_back_uninitialized()) T(std::move(value));
        }

        void clear()
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                // TODO: optimize, too many division operations
                for (uint32_t elementIndex = 0; elementIndex < m_size; ++elementIndex)
                {
                    m_segments[elementIndex / kElementsPerSegment][elementIndex % kElementsPerSegment].~T();
                }
            }

            m_size = 0;
        }

        [[nodiscard]] T** segments() const
        {
            return m_segments;
        }

        [[nodiscard]] uint32_t size() const
        {
            return m_size;
        }

        [[nodiscard]] bool empty() const
        {
            return m_size == 0;
        }

        [[nodiscard]] Iterator<false> begin()
        {
            return { m_segments, 0, 0 };
        }

        [[nodiscard]] Iterator<false> end()
        {
            return { m_segments, m_size / kElementsPerSegment, m_size % kElementsPerSegment };
        }

        [[nodiscard]] Iterator<true> begin() const
        {
            return { m_segments, 0, 0 };
        }

        [[nodiscard]] Iterator<true> end() const
        {
            return { m_segments, m_size / kElementsPerSegment, m_size % kElementsPerSegment };
        }
    };
} // namespace FE
