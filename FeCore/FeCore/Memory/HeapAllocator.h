#pragma once
#include <FeCore/Memory/AllocatorBase.h>
#include <FeCore/Memory/IAllocator.h>

namespace FE
{
    struct HeapAllocatorDesc
    {
        size_t PageSize     = 64 * 1024;
        size_t SpanSize     = 0;
        size_t SpanMapCount = 0;
    };

    class HeapAllocator : public AllocatorBase
    {
        size_t m_TotalUsage = 0;

    public:
        using Desc = HeapAllocatorDesc;

        HeapAllocator();
        inline virtual ~HeapAllocator() {}

        void Init(const Desc& desc);

        //=========================================================================================
        // IAllocator

        virtual void* Allocate(size_t size, size_t alignment, const SourcePosition& position) override;
        virtual void Deallocate(void* pointer, const SourcePosition& position, size_t size = 0) override;
        virtual void* Reallocate(
            void* pointer, const SourcePosition& position, size_t newSize, size_t newAlignment, size_t oldSize = 0) override;
        virtual size_t TotalAllocated() const override;
        virtual size_t SizeOfBlock(void* pointer) override;
        //=========================================================================================
    };
} // namespace FE