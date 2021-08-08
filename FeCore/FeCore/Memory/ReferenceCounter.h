#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Memory/IAllocator.h>
#include <FeCore/Utils/Interlocked.h>
#include <FeCore/RTTI/RTTI.h>

namespace FE
{
    class IObject;

    class ReferenceCounter final
    {
        AtomicInt32 m_StrongRefCount;
        mutable IAllocator* m_Allocator;

    public:
        FE_STRUCT_RTTI(ReferenceCounter, "32BB0481-9163-4E1A-AB7B-A9B6017D9C8D");

        inline ReferenceCounter(IAllocator* allocator)
            : m_StrongRefCount(0)
            , m_Allocator(allocator)
        {
        }

        inline uint32_t AddStrongRef()
        {
            return ++m_StrongRefCount;
        }

        template<class F>
        inline uint32_t ReleaseStrongRef(F&& destroyCallback)
        {
            if (--m_StrongRefCount == 0)
            {
                destroyCallback();
                m_Allocator->Deallocate(this, FE_SRCPOS());
            }

            return m_StrongRefCount;
        }
    };
}
