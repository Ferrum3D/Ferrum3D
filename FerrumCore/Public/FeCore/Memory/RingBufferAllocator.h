#pragma once
#include <FeCore/Base/BaseTypes.h>

namespace FE::Memory
{
    class RingBufferAllocator final
    {
    public:
        struct Handle final
        {
            [[nodiscard]] bool IsValid() const
            {
                return m_size != 0;
            }

            [[nodiscard]] uint32_t GetOffset() const
            {
                return m_offset;
            }

            [[nodiscard]] uint32_t GetSize() const
            {
                // Includes alignment padding consumed by the allocation.
                return m_size;
            }

        private:
            uint32_t m_offset = Constants::kMaxU32;
            uint32_t m_size = 0;

            friend class RingBufferAllocator;
        };

        static_assert(sizeof(Handle) == 8);

        struct Stats final
        {
            uint32_t m_freeBytes = 0;
            uint32_t m_freeBlocks = 0;
            uint32_t m_largestFreeBlock = 0;
        };

        explicit RingBufferAllocator(uint32_t arenaByteSize);

        [[nodiscard]] Handle Allocate(uint32_t size, uint32_t alignment);
        void Free(Handle handle);

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
        uint32_t m_capacity = 0;
        uint32_t m_head = 0;
        uint32_t m_tail = 0;
        uint32_t m_used = 0;
        uint32_t m_wrapBoundary = 0;
        uint32_t m_wrapPadding = 0;
    };
} // namespace FE::Memory
