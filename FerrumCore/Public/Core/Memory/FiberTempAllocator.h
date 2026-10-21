#pragma once
#include <Core/Memory/LinearAllocator.h>

namespace FE::Memory
{
    //! @brief Scoped allocator over the current fiber's linear page; memory is reclaimed when the scope ends.
    //!
    //! Allocations that do not fit a page fall back to the default heap and are freed with the scope. Use only on job fibers.
    struct FiberTempAllocator final : public std::pmr::memory_resource
    {
        FiberTempAllocator();
        ~FiberTempAllocator() override;

        FiberTempAllocator(const FiberTempAllocator&) = delete;
        FiberTempAllocator& operator=(const FiberTempAllocator&) = delete;

    private:
        struct OversizedBlock final
        {
            OversizedBlock* m_next = nullptr;
        };

        void* do_allocate(size_t bytes, size_t alignment) override;
        void do_deallocate(void*, size_t, size_t) override {}

        [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
        {
            return this == &other;
        }

        LinearAllocator* m_allocator;
        LinearAllocator::Marker m_marker;
        OversizedBlock* m_oversized = nullptr;
    };
} // namespace FE::Memory
