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
            [[nodiscard]] uint32_t GetSize() const;

            [[nodiscard]] bool IsValid() const
            {
                return m_offset != Constants::kMaxU32;
            }

            explicit operator bool() const
            {
                return m_offset != Constants::kMaxU32;
            }

            uint32_t m_offset = Constants::kMaxU32;
            uint32_t m_order = 0;
        };

        static_assert(sizeof(Handle) <= 8);

        struct Stats final
        {
            uint32_t m_freeBytes = 0;
            uint32_t m_freeBlocks = 0;
            uint32_t m_largestFreeBlock = 0;
        };

        BuddyAllocator() = default;
        explicit BuddyAllocator(const uint32_t arenaByteSize)
        {
            Setup(arenaByteSize);
        }

        void Setup(uint32_t arenaByteSize);

        [[nodiscard]] Handle Allocate(uint32_t size, uint32_t alignment);
        void Free(Handle handle);
        [[nodiscard]] Stats GetStats() const;

        [[nodiscard]] uint32_t GetArenaSize() const
        {
            return m_arenaSize;
        }

    private:
        static constexpr uint32_t kMinBlockSize = 16;

        uint32_t m_arenaSize = 0;
        uint32_t m_minBlockLog2 = 0;
        uint32_t m_maxLevel = 0;
        festd::vector<festd::bit_vector> m_freeBits;

        [[nodiscard]] static uint32_t BlockSizeForLevel(uint32_t level);
        [[nodiscard]] uint32_t LevelForBlockSize(uint32_t blockSize) const;
    };
} // namespace FE::Memory
