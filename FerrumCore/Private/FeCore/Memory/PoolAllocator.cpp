#include <FeCore/Memory/PoolAllocator.h>

namespace FE::Memory
{
    inline PoolAllocator::Page* PoolAllocator::AllocatePage()
    {
        Page* pResult = static_cast<Page*>(Memory::AllocateVirtual(m_PageByteSize));
        pResult->pNext = m_pPageList;
        pResult->pCurrent = pResult + 1;
        m_pPageList = pResult;
        return pResult;
    }


    inline void* PoolAllocator::AllocateFromPage(Page* pPage) const
    {
        if (!pPage)
            return nullptr;

        const uint8_t* pPageEnd = reinterpret_cast<const uint8_t*>(pPage) + m_PageByteSize;
        const size_t elementByteSize = m_ElementByteSize;

        uint8_t* pResult = static_cast<uint8_t*>(pPage->pCurrent);
        if (pResult + elementByteSize > pPageEnd)
            return nullptr;

        pPage->pCurrent = pResult + elementByteSize;
        return pResult;
    }


    void* PoolAllocator::do_allocate(size_t byteSize, size_t byteAlignment)
    {
        FE_CORE_ASSERT(byteSize <= m_ElementByteSize, "");
        FE_CORE_ASSERT(byteAlignment <= kDefaultAlignment, "");

#if FE_DEBUG
        ++m_AllocationCount;
#endif

        void* pResult;
        if (!m_pFreeList)
        {
            pResult = AllocateFromPage(m_pPageList);
            if (!pResult)
            {
                Page* pPage = AllocatePage();
                pResult = AllocateFromPage(pPage);
            }

            return pResult;
        }

        pResult = m_pFreeList;
        m_pFreeList = *static_cast<void**>(pResult);
        return pResult;
    }


    void PoolAllocator::do_deallocate(void* ptr, size_t, size_t)
    {
        FE_CORE_ASSERT(m_AllocationCount-- > 0, "");
        *static_cast<void**>(ptr) = m_pFreeList;
        m_pFreeList = ptr;
    }
} // namespace FE::Memory
