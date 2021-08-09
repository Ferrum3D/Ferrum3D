#pragma once
#include <FeCore/Memory/AllocatorBase.h>
#include <FeCore/Memory/IAllocator.h>

namespace FE
{
    //! \brief Description of \ref HeapAllocator.
    struct HeapAllocatorDesc
    {
        size_t PageSize     = 64 * 1024;
        size_t SpanSize     = 0;
        size_t SpanMapCount = 0;
    };

    //! \brief Main global heap allocator.
    class HeapAllocator : public AllocatorBase
    {
        size_t m_TotalUsage = 0;

    public:
        FE_CLASS_RTTI(HeapAllocator, "1C0FA67A-09E8-461B-818F-24454F5A5B0B");

        using Desc = HeapAllocatorDesc;

        HeapAllocator();
        inline virtual ~HeapAllocator() = default;

        //! \brief Initialize allocator with provided HeapAllocatorDesc.
        //! 
        //! \param [in] desc - The \ref HeapAllocatorDesc to use.
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
