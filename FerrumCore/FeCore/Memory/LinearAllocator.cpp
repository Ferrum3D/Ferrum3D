#include <FeCore/Memory/LinearAllocator.h>

namespace FE::Memory
{
    void LinearAllocator::NewPage()
    {
        Page* pCurrentPage = m_CurrentMarker.m_pPage;
        if (pCurrentPage == nullptr)
        {
            if (m_pFirstPage == nullptr)
                m_pFirstPage = static_cast<Page*>(m_pPageAllocator->allocate(m_PageByteSize));

            m_CurrentMarker.m_pPage = m_pFirstPage;
            m_CurrentMarker.m_Offset = sizeof(Page);
            return;
        }

        if (pCurrentPage->pNext == nullptr)
        {
            Page* pNewPage = static_cast<Page*>(m_pPageAllocator->allocate(m_PageByteSize));
            pCurrentPage->pNext = pNewPage;
        }

        m_CurrentMarker.m_pPage = pCurrentPage->pNext;
        m_CurrentMarker.m_Offset = sizeof(Page);
    }


    LinearAllocator::LinearAllocator(size_t pageByteSize, std::pmr::memory_resource* pPageAllocator)
        : m_PageByteSize(pageByteSize)
        , m_pPageAllocator(pPageAllocator)
    {
        FE_CORE_ASSERT(pageByteSize > 0, "Page size must be greater than zero");
        if (m_pPageAllocator == nullptr)
        {
            m_pPageAllocator = std::pmr::get_default_resource();
        }
    }


    LinearAllocator::~LinearAllocator()
    {
        Page* pPage = m_pFirstPage;
        m_pFirstPage = nullptr;

        while (pPage)
        {
            Page* pOldPage = pPage;
            pPage = pPage->pNext;
            m_pPageAllocator->deallocate(pOldPage, m_PageByteSize);
        }
    }


    void LinearAllocator::Collect()
    {
        Page* pPage = m_CurrentMarker.m_pPage;
        if (pPage == nullptr)
            return;

        pPage = pPage->pNext;
        m_CurrentMarker.m_pPage->pNext = nullptr;

        while (pPage)
        {
            Page* pOldPage = pPage;
            pPage = pPage->pNext;
            m_pPageAllocator->deallocate(pOldPage, m_PageByteSize);
        }
    }
} // namespace FE::Memory
