#pragma once
#include <FeCore/Base/BaseTypes.h>
#include <festd/bit_vector.h>
#include <festd/vector.h>

namespace FE::Memory
{
    struct BuddyAllocator final
    {
        struct Handle final
        {
            [[nodiscard]] bool IsValid() const
            {
                return m_offset != Constants::kMaxU32;
            }

            void Invalidate()
            {
                m_offset = Constants::kMaxU32;
                m_level = 0;
            }

            explicit operator bool() const
            {
                return m_offset != Constants::kMaxU32;
            }

            uint32_t m_offset = Constants::kMaxU32;
            uint32_t m_level = 0;
        };

        static_assert(sizeof(Handle) <= 8);

        struct Stats final
        {
            uint32_t m_freeBytes = 0;
            uint32_t m_freeBlocks = 0;
            uint32_t m_largestFreeBlock = 0;
        };

        BuddyAllocator() = default;
        explicit BuddyAllocator(const uint32_t arenaByteSize, const uint32_t minBlockSize = 1)
        {
            Setup(arenaByteSize, minBlockSize);
        }

        void Setup(uint32_t arenaByteSize, uint32_t minBlockSize = 1);
        void Shutdown();

        [[nodiscard]] Handle Allocate(uint32_t size, uint32_t alignment = 0);
        void Free(Handle handle);

        [[nodiscard]] uint32_t GetUsableSize(Handle handle) const;
        [[nodiscard]] uint32_t QuantizeAllocationSize(uint32_t size) const;

        [[nodiscard]] Stats GetStats() const;

        [[nodiscard]] uint32_t GetArenaSize() const
        {
            return m_arenaSize;
        }

        [[nodiscard]] uint32_t BlockSizeForLevel(uint32_t level) const;
        [[nodiscard]] uint32_t LevelForBlockSize(uint32_t blockSize) const;

    private:
        uint32_t m_arenaSize = 0;
        uint32_t m_minBlockSize = 0;
        uint32_t m_minBlockLog2 = 0;
        uint32_t m_maxLevel = 0;
        festd::vector<festd::bit_vector> m_freeBits;
    };
} // namespace FE::Memory
