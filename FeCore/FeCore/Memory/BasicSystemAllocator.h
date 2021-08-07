#pragma once
#include <FeCore/Memory/IBasicAllocator.h>
#include <FeCore/Base/Platform.h>

namespace FE
{
    /**
     * @brief IBasicAllocator implementation that uses aligned versions malloc() and free().
    */
    class BasicSystemAllocator : public IBasicAllocator
    {
    public:
        virtual void* Allocate(size_t size, size_t alignment) noexcept override
        {
            return FE_ALIGNED_MALLOC(size, alignment);
        }

        virtual void Deallocate(void* ptr) noexcept override
        {
            FE_ALIGNED_FREE(ptr);
        }
    };
} // namespace FE
