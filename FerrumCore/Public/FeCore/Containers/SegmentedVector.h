#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE
{
    template<class T, size_t TSegmentByteSize = 4096>
    class SegmentedVector final
    {
        template<class TOther, size_t TSizeOther>
        friend class SegmentedVector;

        inline static uint32_t ElementsPerSegment = TSegmentByteSize / sizeof(T);

        static_assert(sizeof(T) <= TSegmentByteSize);

        std::pmr::memory_resource* m_pAllocator = nullptr;
        T** m_ppSegments = nullptr;
        uint32_t m_SegmentTableSize = 0;
        uint32_t m_Size = 0;

        inline void GrowSegmentTable()
        {
            const uint32_t newSize = m_SegmentTableSize == 0 ? 1 : m_SegmentTableSize * 2;
            T** ppNewTable = Memory::AllocateArray<T*>(m_pAllocator, newSize);
            memset(ppNewTable + m_SegmentTableSize, 0, static_cast<size_t>(newSize - m_SegmentTableSize) * sizeof(T*));

            if (m_ppSegments)
            {
                memcpy(ppNewTable, m_ppSegments, m_SegmentTableSize * sizeof(T*));
                m_pAllocator->deallocate(m_ppSegments, sizeof(T*) * m_SegmentTableSize);
            }

            m_SegmentTableSize = newSize;
            m_ppSegments = ppNewTable;
        }

        inline void ResetImpl()
        {
            if (!m_ppSegments)
                return;

            clear();

            for (uint32_t segmentIndex = 0; segmentIndex < m_SegmentTableSize; ++segmentIndex)
            {
                if (m_ppSegments[segmentIndex] == nullptr)
                    break;

                m_pAllocator->deallocate(m_ppSegments[segmentIndex], TSegmentByteSize, alignof(T));
            }

            m_pAllocator->deallocate(m_ppSegments, sizeof(T*) * m_SegmentTableSize, alignof(T*));
            m_ppSegments = nullptr;
        }

    public:
        template<bool TConst>
        class Iterator final
        {
            friend class SegmentedVector;

            T** m_ppSegments = nullptr;
            uint32_t m_SegmentIndex = 0;
            uint32_t m_ElementIndex = 0;

            inline Iterator(T** ppSegments, uint32_t segmentIndex, uint32_t elementIndex)
                : m_ppSegments(ppSegments)
                , m_SegmentIndex(segmentIndex)
                , m_ElementIndex(elementIndex)
            {
            }

        public:
            using difference_type = int32_t;
            using value_type = T;
            using pointer = std::conditional_t<TConst, const T*, T*>;
            using reference = std::conditional_t<TConst, const T&, T&>;

            inline reference operator*() const
            {
                return m_ppSegments[m_SegmentIndex][m_ElementIndex];
            }

            inline pointer operator->() const
            {
                return &m_ppSegments[m_SegmentIndex][m_ElementIndex];
            }

            inline Iterator& operator++()
            {
                if (m_ElementIndex < ElementsPerSegment - 1)
                {
                    ++m_ElementIndex;
                }
                else
                {
                    ++m_SegmentIndex;
                    m_ElementIndex = 0;
                }

                return *this;
            }

            inline Iterator operator++(int)
            {
                Iterator t = *this;
                ++(*this);
                return t;
            }

            inline Iterator& operator--()
            {
                if (m_ElementIndex > 0)
                {
                    --m_ElementIndex;
                }
                else
                {
                    FE_CORE_ASSERT(m_SegmentIndex > 0, "Iterator out of range");
                    --m_SegmentIndex;
                    m_ElementIndex = ElementsPerSegment - 1;
                }

                return *this;
            }

            inline Iterator operator--(int)
            {
                Iterator t = *this;
                --(*this);
                return t;
            }

            inline Iterator operator+(int32_t rhs) const
            {
                const uint32_t initialIndex = m_SegmentIndex * ElementsPerSegment + m_ElementIndex;
                const uint32_t newIndex = initialIndex + rhs;
                return Iterator{ m_ppSegments, newIndex / ElementsPerSegment, newIndex % ElementsPerSegment };
            }

            inline Iterator operator-(int32_t rhs) const
            {
                const uint32_t initialIndex = m_SegmentIndex * ElementsPerSegment + m_ElementIndex;
                const uint32_t newIndex = initialIndex - rhs;
                return Iterator{ m_ppSegments, newIndex / ElementsPerSegment, newIndex % ElementsPerSegment };
            }

            inline Iterator& operator+=(int32_t rhs)
            {
                const uint32_t initialIndex = m_SegmentIndex * ElementsPerSegment + m_ElementIndex;
                const uint32_t newIndex = initialIndex + rhs;
                m_SegmentIndex = newIndex / ElementsPerSegment;
                m_ElementIndex = newIndex % ElementsPerSegment;
                return *this;
            }

            inline Iterator& operator-=(int32_t rhs)
            {
                const uint32_t initialIndex = m_SegmentIndex * ElementsPerSegment + m_ElementIndex;
                const uint32_t newIndex = initialIndex - rhs;
                m_SegmentIndex = newIndex / ElementsPerSegment;
                m_ElementIndex = newIndex % ElementsPerSegment;
                return *this;
            }

            inline Iterator operator-(const Iterator& other) const
            {
                const uint32_t lhsIndex = m_SegmentIndex * ElementsPerSegment + m_ElementIndex;
                const uint32_t rhsIndex = other.m_SegmentIndex * ElementsPerSegment + other.m_ElementIndex;
                return static_cast<int32_t>(lhsIndex) - static_cast<int32_t>(rhsIndex);
            }

            inline friend bool operator==(const Iterator& lhs, const Iterator& rhs)
            {
                return lhs.m_SegmentIndex == rhs.m_SegmentIndex && lhs.m_ElementIndex == rhs.m_ElementIndex;
            }

            inline friend bool operator!=(const Iterator& lhs, const Iterator& rhs)
            {
                return !(lhs == rhs);
            }
        };

        inline SegmentedVector(std::pmr::memory_resource* pAllocator = nullptr)
            : m_pAllocator(pAllocator)
        {
            if (m_pAllocator == nullptr)
                m_pAllocator = std::pmr::get_default_resource();
        }

        inline ~SegmentedVector()
        {
            ResetImpl();
        }

        inline SegmentedVector(const SegmentedVector& other)
            : m_pAllocator(other.m_pAllocator)
        {
            const uint32_t segmentCount = Math::CeilDivide(other.m_Size, ElementsPerSegment);
            m_SegmentTableSize = segmentCount;
            m_ppSegments = Memory::AllocateArray<T*>(m_pAllocator, segmentCount);
            memset(m_ppSegments, 0, sizeof(T*) * segmentCount);
            for (const T& element : other)
            {
                push_back(element);
            }
        }

        template<size_t TSizeOther>
        inline SegmentedVector(const SegmentedVector<T, TSizeOther>& other)
            : m_pAllocator(other.m_pAllocator)
        {
            const uint32_t segmentCount = Math::CeilDivide(other.m_Size, ElementsPerSegment);
            m_SegmentTableSize = segmentCount;
            m_ppSegments = Memory::AllocateArray<T*>(m_pAllocator, segmentCount);
            memset(m_ppSegments, 0, sizeof(T*) * segmentCount);
            for (const T& element : other)
            {
                push_back(element);
            }
        }

        inline SegmentedVector& operator=(const SegmentedVector& other)
        {
            ResetImpl();
            new (this) SegmentedVector(other);
            return *this;
        }

        template<size_t TSizeOther>
        inline SegmentedVector& operator=(const SegmentedVector<T, TSizeOther>& other)
        {
            ResetImpl();
            new (this) SegmentedVector(other);
            return *this;
        }

        inline SegmentedVector(SegmentedVector&& other) noexcept
            : m_pAllocator(other.m_pAllocator)
            , m_ppSegments(other.m_ppSegments)
            , m_SegmentTableSize(other.m_SegmentTableSize)
            , m_Size(other.m_Size)
        {
            other.m_ppSegments = nullptr;
            other.m_Size = 0;
            other.m_SegmentTableSize = 0;
        }

        inline SegmentedVector& operator=(SegmentedVector&& other) noexcept
        {
            ResetImpl();
            new (this) SegmentedVector(std::move(other));
            return *this;
        }

        [[nodiscard]] inline T& operator[](uint32_t index)
        {
            FE_CORE_ASSERT(index < m_Size, "Index out of range");

            const uint32_t segmentIndex = index / ElementsPerSegment;
            const uint32_t elementIndex = index % ElementsPerSegment;
            return m_ppSegments[segmentIndex][elementIndex];
        }

        [[nodiscard]] inline const T& operator[](uint32_t index) const
        {
            FE_CORE_ASSERT(index < m_Size, "Index out of range");

            const uint32_t segmentIndex = index / ElementsPerSegment;
            const uint32_t elementIndex = index % ElementsPerSegment;
            return m_ppSegments[segmentIndex][elementIndex];
        }

        inline void* push_back_uninitialized()
        {
            const uint32_t segmentIndex = m_Size / ElementsPerSegment;
            const uint32_t elementIndex = m_Size % ElementsPerSegment;

            if (m_SegmentTableSize <= segmentIndex)
                GrowSegmentTable();

            if (elementIndex == 0)
            {
                if (m_ppSegments[segmentIndex] == nullptr)
                    m_ppSegments[segmentIndex] = static_cast<T*>(m_pAllocator->allocate(TSegmentByteSize, alignof(T)));
            }
            else
            {
                FE_CORE_ASSERT(m_ppSegments[segmentIndex], "");
            }

            ++m_Size;
            return &m_ppSegments[segmentIndex][elementIndex];
        }

        inline T& push_back()
        {
            T* result = new (push_back_uninitialized()) T{};
            return *result;
        }

        inline void push_back(const T& value)
        {
            new (push_back_uninitialized()) T(value);
        }

        inline void push_back(T&& value)
        {
            new (push_back_uninitialized()) T(std::move(value));
        }

        inline void clear()
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (uint32_t elementIndex = 0; elementIndex < m_Size; ++elementIndex)
                {
                    m_ppSegments[elementIndex / ElementsPerSegment][elementIndex % ElementsPerSegment].~T();
                }
            }

            m_Size = 0;
        }

        [[nodiscard]] inline T** segments() const
        {
            return m_ppSegments;
        }

        [[nodiscard]] inline uint32_t size() const
        {
            return m_Size;
        }

        [[nodiscard]] inline bool empty() const
        {
            return m_Size == 0;
        }

        [[nodiscard]] inline Iterator<false> begin()
        {
            return { m_ppSegments, 0, 0 };
        }

        [[nodiscard]] inline Iterator<false> end()
        {
            return { m_ppSegments, m_Size / ElementsPerSegment, m_Size % ElementsPerSegment };
        }

        [[nodiscard]] inline Iterator<true> begin() const
        {
            return { m_ppSegments, 0, 0 };
        }

        [[nodiscard]] inline Iterator<true> end() const
        {
            return { m_ppSegments, m_Size / ElementsPerSegment, m_Size % ElementsPerSegment };
        }
    };
} // namespace FE
