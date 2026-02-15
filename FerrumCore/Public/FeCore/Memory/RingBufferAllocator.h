#pragma once
#include <FeCore/Base/BaseTypes.h>

namespace FE::Memory
{
    struct RingBufferAllocator final
    {
        struct Handle final
        {
            [[nodiscard]] bool IsValid() const
            {
                return m_size != 0;
            }

            explicit operator bool() const
            {
                return m_size != 0;
            }

            uint32_t m_offset = Constants::kMaxU32;
            uint32_t m_size = 0;
        };

        static_assert(sizeof(Handle) == 8);

        struct Stats final
        {
            uint32_t m_freeBytes = 0;
            uint32_t m_largestFreeBlock = 0;
        };

        RingBufferAllocator() = default;
        explicit RingBufferAllocator(const uint32_t capacity)
        {
            Setup(capacity);
        }

        void Setup(uint32_t capacity);

        [[nodiscard]] Handle Allocate(uint32_t size, uint32_t alignment);
        void Free(Handle handle);
        void Free(uint32_t bytes);

        [[nodiscard]] Stats GetStats() const;

        [[nodiscard]] uint32_t GetArenaSize() const
        {
            return m_capacity;
        }

        [[nodiscard]] uint32_t GetUsedBytes() const
        {
            return m_used;
        }

    private:
        static constexpr Handle kInvalidHandle = {};

        uint32_t GetTail() const;
        uint32_t CalculateWrapPadding(uint32_t tail, uint32_t size) const;

        uint32_t m_capacity = 0;
        uint32_t m_head = 0;
        uint32_t m_used = 0;
    };
} // namespace FE::Memory
