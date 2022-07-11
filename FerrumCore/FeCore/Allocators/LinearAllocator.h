#pragma once
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/AllocatorBase.h>
#include <FeCore/Memory/HeapAllocator.h>

namespace FE
{
    template<class THandle>
    struct GenericLinearAllocatorDesc
    {
        THandle StartOffset;
        USize Capacity;
        Int32 GCLatency;
    };

    template<class THandle>
    class GenericLinearAllocator : public GenericAllocatorBase<THandle>
    {
        GenericLinearAllocatorDesc<THandle> m_Desc;
        THandle m_CurrentOffset{};
        Int32 m_GCIteration = 0;

        using HandleTraits = AllocationHandleTraits<THandle>;

    public:
        using Desc = GenericLinearAllocatorDesc<THandle>;

        inline GenericLinearAllocator()
            : GenericAllocatorBase<THandle>("Linear allocator", "Local allocator that allocates from a contiguous block")
        {
        }

        inline void Init(const Desc& desc)
        {
            m_Desc          = desc;
            m_CurrentOffset = desc.StartOffset;
        }

        inline THandle Allocate(USize size, USize alignment, [[maybe_unused]] const SourcePosition& position) override
        {
            FE_PROFILE_ALLOC(size, alignment, position);

            if (size == 0)
            {
                return THandle{};
            }

            auto alignedPtr  = AlignUp<THandle, USize>(m_CurrentOffset, alignment);
            auto sizeAligned = AlignUp<USize>(size, alignment);
            auto address     = HandleTraits::Advance(alignedPtr, sizeAligned);

            if (address > HandleTraits::Advance(m_Desc.StartOffset, m_Desc.Capacity))
            {
                return THandle{};
            }

            m_CurrentOffset = address;
            return address;
        }

        inline void Deallocate(THandle /* pointer */, const SourcePosition& /* position */, USize /* size */) override
        {
            // Do nothing here, the memory is freed only from CollectGarbage();
        }

        inline THandle Reallocate(
            THandle /* pointer */, const SourcePosition& position, USize newSize, USize newAlignment,
            USize /* oldSize */) override
        {
            return Allocate(newSize, newAlignment, position);
        }

        inline USize SizeOfBlock(THandle) override
        {
            return 0;
        }

        [[nodiscard]] inline USize TotalAllocated() const override
        {
            return HandleTraits::Difference(m_CurrentOffset, m_Desc.StartOffset);
        }

        inline void CollectGarbage() override
        {
            if (m_GCIteration >= m_Desc.GCLatency)
            {
                CollectGarbageForce();
                m_GCIteration = 0;
            }
            else
            {
                m_GCIteration++;
            }
        }

        inline void CollectGarbageForce()
        {
            m_CurrentOffset = m_Desc.StartOffset;
        }
    };

    using LinearAllocatorDesc = GenericLinearAllocatorDesc<void*>;
    using LinearAllocator     = GenericLinearAllocator<void*>;
} // namespace FE
