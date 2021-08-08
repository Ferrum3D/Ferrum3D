#pragma once
#include <FeCore/Base/Platform.h>
#include <FeCore/Memory/IBasicAllocator.h>
#include <FeCore/RTTI/RTTI.h>

namespace FE
{
    /**
     * @brief IBasicAllocator implementation that uses aligned versions malloc() and free().
    */
    class BasicSystemAllocator : public IBasicAllocator
    {
    public:
        FE_CLASS_RTTI(BasicSystemAllocator, "3C61313A-C682-4142-ABB8-6D053868397A");

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
