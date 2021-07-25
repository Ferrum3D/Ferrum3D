#pragma once
#include "IBasicAllocator.h"
#include <Utils/Platform.h>

namespace FE
{
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
