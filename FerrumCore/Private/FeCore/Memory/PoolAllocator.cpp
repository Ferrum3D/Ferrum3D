#include <FeCore/Memory/PoolAllocator.h>

namespace FE::Memory
{
    inline PoolAllocator::Page* PoolAllocator::AllocatePage()
    {
        Page* pResult = static_cast<Page*>(AllocateVirtual(m_pageByteSize));
        pResult->m_next = m_pageList;
        pResult->m_current = pResult + 1;
        m_pageList = pResult;
        return pResult;
    }


    inline void* PoolAllocator::AllocateFromPage(Page* pPage) const
    {
        if (!pPage)
            return nullptr;

        const uint8_t* pPageEnd = reinterpret_cast<const uint8_t*>(pPage) + m_pageByteSize;
        const size_t elementByteSize = m_elementByteSize;

        uint8_t* pResult = static_cast<uint8_t*>(pPage->m_current);
        if (pResult + elementByteSize > pPageEnd)
            return nullptr;

        pPage->m_current = pResult + elementByteSize;
        return pResult;
    }


    void* PoolAllocator::do_allocate(size_t byteSize, size_t byteAlignment)
    {
        FE_CORE_ASSERT(byteSize <= m_elementByteSize, "");
        FE_CORE_ASSERT(byteAlignment <= kDefaultAlignment, "");

#if FE_DEBUG
        ++m_allocationCount;
#endif

        void* pResult;
        if (!m_freeList)
        {
            pResult = AllocateFromPage(m_pageList);
            if (!pResult)
            {
                Page* pPage = AllocatePage();
                pResult = AllocateFromPage(pPage);
            }

            return pResult;
        }

        pResult = m_freeList;
        m_freeList = *static_cast<void**>(pResult);
        return pResult;
    }


    void PoolAllocator::do_deallocate(void* ptr, size_t, size_t)
    {
        FE_CORE_ASSERT(m_allocationCount-- > 0, "");
        *static_cast<void**>(ptr) = m_freeList;
        m_freeList = ptr;
    }
} // namespace FE::Memory
