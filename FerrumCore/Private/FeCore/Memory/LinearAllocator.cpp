#include <FeCore/Memory/LinearAllocator.h>

namespace FE::Memory
{
    void LinearAllocator::NewPage()
    {
        Page* pCurrentPage = m_currentMarker.m_page;
        if (pCurrentPage == nullptr)
        {
            if (m_firstPage == nullptr)
            {
                m_firstPage = static_cast<Page*>(m_pageAllocator->allocate(m_pageByteSize));
                m_firstPage->pNext = nullptr;
            }

            m_currentMarker.m_page = m_firstPage;
            m_currentMarker.m_offset = sizeof(Page);
            return;
        }

        if (pCurrentPage->pNext == nullptr)
        {
            Page* pNewPage = static_cast<Page*>(m_pageAllocator->allocate(m_pageByteSize));
            pCurrentPage->pNext = pNewPage;
        }

        m_currentMarker.m_page = pCurrentPage->pNext;
        m_currentMarker.m_offset = sizeof(Page);
    }


    LinearAllocator::LinearAllocator(const size_t pageByteSize, std::pmr::memory_resource* pPageAllocator)
        : m_pageByteSize(pageByteSize)
        , m_pageAllocator(pPageAllocator)
    {
        FE_CoreAssert(pageByteSize > 0, "Page size must be greater than zero");
        if (m_pageAllocator == nullptr)
        {
            m_pageAllocator = std::pmr::get_default_resource();
        }
    }


    LinearAllocator::~LinearAllocator()
    {
        Page* pPage = m_firstPage;
        m_firstPage = nullptr;

        while (pPage)
        {
            Page* pOldPage = pPage;
            pPage = pPage->pNext;
            m_pageAllocator->deallocate(pOldPage, m_pageByteSize);
        }
    }


    void LinearAllocator::Collect()
    {
        Page* pPage = m_currentMarker.m_page;
        if (pPage == nullptr)
            return;

        pPage = pPage->pNext;
        m_currentMarker.m_page->pNext = nullptr;

        while (pPage)
        {
            Page* pOldPage = pPage;
            pPage = pPage->pNext;
            m_pageAllocator->deallocate(pOldPage, m_pageByteSize);
        }
    }
} // namespace FE::Memory
