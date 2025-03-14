#pragma once
#include <festd/Internal/StringBase.h>
#include <memory_resource>

namespace FE::Internal
{
    FE_FORCE_INLINE constexpr uint32_t GetNewStringCapacity(const uint32_t currentCapacity, const uint32_t minimum)
    {
        return Math::Max(minimum, currentCapacity * 2);
    }


    struct DynamicStringStorage
    {
        static constexpr uint32_t kShortModeCapacity = 23;

        struct Long final
        {
            char* m_data;
            uint32_t m_size;
            uint32_t m_capacity;
            uint8_t m_padding[7];
            uint8_t m_marker;
        };

        struct Short final
        {
            char m_data[kShortModeCapacity];
            uint8_t m_invSize;
        };

        union
        {
            Long m_long;
            Short m_short;
            uint64_t m_words[3];
        };

        [[nodiscard]] bool IsLong() const
        {
            return m_long.m_marker == 0xff;
        }

        [[nodiscard]] uint32_t GetShortSize() const
        {
            return m_short.m_invSize ^ kShortModeCapacity;
        }

        [[nodiscard]] uint32_t SizeImpl() const
        {
            return IsLong() ? m_long.m_size : GetShortSize();
        }

        [[nodiscard]] uint32_t CapacityImpl() const
        {
            return IsLong() ? m_long.m_capacity : kShortModeCapacity;
        }

        [[nodiscard]] const char* DataImpl() const
        {
            return IsLong() ? m_long.m_data : m_short.m_data;
        }

        [[nodiscard]] char* DataImpl()
        {
            return IsLong() ? m_long.m_data : m_short.m_data;
        }

        template<bool TKeepData>
        FE_FORCE_INLINE char* GrowImpl(const uint32_t length, const uint32_t capacity, const bool isLong,
                                       std::pmr::memory_resource* allocator)
        {
            if (isLong)
            {
                if (m_long.m_capacity >= length)
                {
                    m_long.m_size = length;
                    return m_long.m_data;
                }

                char* newData = Memory::AllocateArray<char>(allocator, capacity);
                if (TKeepData)
                    memcpy(newData, m_long.m_data, m_long.m_size);

                allocator->deallocate(m_long.m_data, m_long.m_capacity + 1);

                m_long.m_capacity = capacity - 1;
                m_long.m_data = newData;
                m_long.m_size = length;
                return m_long.m_data;
            }
            else
            {
                if (capacity <= kShortModeCapacity)
                {
                    m_short.m_invSize = static_cast<uint8_t>(length ^ kShortModeCapacity);
                    return m_short.m_data;
                }

                char* newData = Memory::AllocateArray<char>(allocator, capacity);
                if (TKeepData)
                    memcpy(newData, m_short.m_data, GetShortSize());

                m_long.m_marker = 0xff;
                m_long.m_data = newData;
                m_long.m_capacity = capacity - 1;
                m_long.m_size = length;
                return m_long.m_data;
            }
        }

        char* InitializeImpl(const uint32_t length, std::pmr::memory_resource* allocator)
        {
            return GrowImpl<false>(length, GetNewStringCapacity(0, length + 1), false, allocator);
        }

        char* Reinitialize(const uint32_t length, std::pmr::memory_resource* allocator)
        {
            return GrowImpl<false>(length, GetNewStringCapacity(0, length + 1), IsLong(), allocator);
        }

        char* ReserveImpl(const uint32_t length, std::pmr::memory_resource* allocator)
        {
            const bool isLong = IsLong();
            const uint32_t size = isLong ? m_long.m_size : GetShortSize();
            const uint32_t oldCapacity = isLong ? m_long.m_capacity : kShortModeCapacity;
            const uint32_t capacity = GetNewStringCapacity(oldCapacity, length + 1);
            return GrowImpl<true>(size, capacity, isLong, allocator);
        }

        char* ResizeImpl(const uint32_t length, std::pmr::memory_resource* allocator)
        {
            const bool isLong = IsLong();
            const uint32_t oldCapacity = isLong ? m_long.m_capacity : kShortModeCapacity;
            const uint32_t capacity = GetNewStringCapacity(oldCapacity, length + 1);
            return GrowImpl<true>(length, capacity, isLong, allocator);
        }

        void ShrinkImpl(std::pmr::memory_resource* allocator)
        {
            if (!IsLong())
                return;

            const uint32_t size = m_long.m_size;
            if (size < m_long.m_capacity)
            {
                if (size <= kShortModeCapacity)
                {
                    char* oldData = m_long.m_data;
                    memcpy(m_short.m_data, oldData, size);
                    m_short.m_invSize = static_cast<uint8_t>(size ^ kShortModeCapacity);
                    return;
                }

                char* newData = Memory::AllocateArray<char>(allocator, size + 1);
                memcpy(newData, m_long.m_data, size);
                allocator->deallocate(m_long.m_data, size);

                m_long.m_data = newData;
                m_long.m_capacity = size;
            }
        }

        void DestroyImpl(std::pmr::memory_resource* allocator)
        {
            if (IsLong())
            {
                allocator->deallocate(m_long.m_data, m_long.m_capacity + 1);
                m_short.m_invSize = static_cast<uint8_t>(0 ^ kShortModeCapacity);
            }
        }
    };


    template<uint32_t TCapacity>
    struct FixedStringStorage
    {
        static constexpr uint32_t kCapacity = TCapacity;

        static_assert(TCapacity <= (1 << 24));

        using SizeBaseType = std::conditional_t<TCapacity <= 255, uint16_t, uint32_t>;

        char m_data[TCapacity];
        SizeBaseType m_zero : 8;
        SizeBaseType m_size : (sizeof(SizeBaseType) - 1) * 8;

        [[nodiscard]] uint32_t SizeImpl() const
        {
            return m_size;
        }

        [[nodiscard]] uint32_t CapacityImpl() const
        {
            return kCapacity;
        }

        [[nodiscard]] const char* DataImpl() const
        {
            return m_data;
        }

        char* DataImpl()
        {
            return m_data;
        }

        char* InitializeImpl(const uint32_t length, std::pmr::memory_resource*)
        {
            FE_Assert(length <= kCapacity, "Fixed string overflow");
            m_size = static_cast<SizeBaseType>(length);
            return m_data;
        }

        char* Reinitialize(const uint32_t length, std::pmr::memory_resource*)
        {
            FE_Assert(length <= kCapacity, "Fixed string overflow");
            m_size = static_cast<SizeBaseType>(length);
            return m_data;
        }

        char* ReserveImpl(const uint32_t length, std::pmr::memory_resource*)
        {
            FE_Assert(length <= kCapacity, "Fixed string overflow");
            return m_data;
        }

        char* ResizeImpl(const uint32_t length, std::pmr::memory_resource*)
        {
            FE_Assert(length <= kCapacity, "Fixed string overflow");
            m_size = static_cast<SizeBaseType>(length);
            return m_data;
        }

        void ShrinkImpl(std::pmr::memory_resource*) {}

        void DestroyImpl(std::pmr::memory_resource*) {}
    };
} // namespace FE::Internal
