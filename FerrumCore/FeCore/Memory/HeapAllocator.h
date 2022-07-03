#pragma once
#include <FeCore/Memory/AllocatorBase.h>
#include <FeCore/Memory/IAllocator.h>
#include <FeCore/Parallel/Interlocked.h>

namespace FE
{
    //! \brief Description of \ref HeapAllocator.
    struct HeapAllocatorDesc
    {
        USize PageSize     = 64 * 1024;
        USize SpanSize     = 0;
        USize SpanMapCount = 0;
    };

    //! \brief Main global heap allocator.
    class HeapAllocator final : public AllocatorBase
    {
        AtomicInt64 m_TotalUsage;

    public:
        FE_CLASS_RTTI(HeapAllocator, "1C0FA67A-09E8-461B-818F-24454F5A5B0B");

        using Desc = HeapAllocatorDesc;

        HeapAllocator();
        inline ~HeapAllocator() noexcept override;

        //! \brief Initialize allocator with provided HeapAllocatorDesc.
        //!
        //! \param [in] desc - The \ref HeapAllocatorDesc to use.
        void Init(const Desc& desc);
        void ThreadInit();

        //=========================================================================================
        // IAllocator

        [[nodiscard]] void* Allocate(USize size, USize alignment, const SourcePosition& position) override;
        void Deallocate(void* pointer, const SourcePosition& position, USize size) override;
        [[nodiscard]] void* Reallocate(
            void* pointer, const SourcePosition& position, USize newSize, USize newAlignment, USize oldSize) override;
        [[nodiscard]] USize TotalAllocated() const override;
        [[nodiscard]] USize SizeOfBlock(void* pointer) override;
        //=========================================================================================
    };
} // namespace FE
