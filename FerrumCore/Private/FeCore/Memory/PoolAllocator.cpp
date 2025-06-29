#include <FeCore/Memory/PoolAllocator.h>

namespace FE::Memory
{
    inline PoolAllocator::Page* PoolAllocator::AllocatePage()
    {
        Page* result = static_cast<Page*>(AllocateVirtual(m_pageByteSize));
        result->m_next = m_pageList;
        result->m_current = result + 1;
        m_pageList = result;
        return result;
    }


    inline void* PoolAllocator::AllocateFromPage(Page* page) const
    {
        if (!page)
            return nullptr;

        const uint8_t* pageEnd = reinterpret_cast<const uint8_t*>(page) + m_pageByteSize;
        const size_t elementByteSize = m_elementByteSize;

        uint8_t* result = static_cast<uint8_t*>(page->m_current);
        if (result + elementByteSize > pageEnd)
            return nullptr;

        page->m_current = result + elementByteSize;
        return result;
    }


    void* PoolAllocator::do_allocate(const size_t byteSize, const size_t byteAlignment)
    {
        FE_CoreAssert(m_elementByteSize > 0, "Pool must be initialized");
        FE_CoreAssert(byteSize <= m_elementByteSize);
        FE_CoreAssert(byteAlignment <= kDefaultAlignment);

#if FE_DEBUG
        ++m_allocationCount;
#endif

        void* result;
        if (!m_freeList)
        {
            result = AllocateFromPage(m_pageList);
            if (!result)
            {
                Page* page = AllocatePage();
                result = AllocateFromPage(page);
            }

            TracySecureAllocNS(result, m_elementByteSize, 32, m_name);
            return result;
        }

        result = m_freeList;
        m_freeList = *static_cast<void**>(result);
        TracySecureAllocNS(result, m_elementByteSize, 32, m_name);
        return result;
    }


    void PoolAllocator::do_deallocate(void* ptr, size_t, size_t)
    {
        FE_CoreAssertDebug(m_allocationCount > 0);

#if FE_DEBUG
        --m_allocationCount;
#endif

        *static_cast<void**>(ptr) = m_freeList;
        m_freeList = ptr;

        TracySecureFreeNS(ptr, 32, m_name);
    }
} // namespace FE::Memory
