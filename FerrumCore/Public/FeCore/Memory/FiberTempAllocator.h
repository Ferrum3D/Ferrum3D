#pragma once
#include <FeCore/Memory/LinearAllocator.h>

namespace FE::Memory
{
    struct FiberTempAllocator final : public std::pmr::memory_resource
    {
        FiberTempAllocator();
        ~FiberTempAllocator() override;

    private:
        void* do_allocate(size_t bytes, size_t alignment) override;
        void do_deallocate(void*, size_t, size_t) override {}

        [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
        {
            return this == &other;
        }

        LinearAllocator* m_allocator;
        LinearAllocator::Marker m_marker;
    };
} // namespace FE::Memory
