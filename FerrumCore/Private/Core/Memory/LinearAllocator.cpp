#include <Core/Memory/LinearAllocator.h>

namespace FE::Memory
{
    LinearAllocator::LinearAllocator(LinearAllocator&& other) noexcept
    {
        swap(*this, other);
    }


    LinearAllocator& LinearAllocator::operator=(LinearAllocator&& other) noexcept
    {
        swap(*this, other);
        return *this;
    }


    void LinearAllocator::Invalidate()
    {
        m_pageByteSize = 0;
        m_pageAllocator = nullptr;
        m_currentMarker = {};
        m_firstPage = nullptr;
    }


    void LinearAllocator::Destroy()
    {
        Page* page = m_firstPage;
        m_firstPage = nullptr;

        while (page)
        {
            Page* oldPage = page;
            page = page->m_next;
            m_pageAllocator->deallocate(oldPage, m_pageByteSize);
        }
    }


    void swap(LinearAllocator& lhs, LinearAllocator& rhs) noexcept
    {
        festd::swap(lhs.m_pageByteSize, rhs.m_pageByteSize);
        festd::swap(lhs.m_pageAllocator, rhs.m_pageAllocator);
        festd::swap(lhs.m_currentMarker, rhs.m_currentMarker);
        festd::swap(lhs.m_firstPage, rhs.m_firstPage);
    }


    void LinearAllocator::NewPage()
    {
        Page* currentPage = m_currentMarker.m_page;
        if (currentPage == nullptr)
        {
            if (m_firstPage == nullptr)
            {
                m_firstPage = static_cast<Page*>(m_pageAllocator->allocate(m_pageByteSize));
                m_firstPage->m_next = nullptr;
            }

            m_currentMarker.m_page = m_firstPage;
            m_currentMarker.m_offset = sizeof(Page);
            return;
        }

        if (currentPage->m_next == nullptr)
        {
            auto* newPage = static_cast<Page*>(m_pageAllocator->allocate(m_pageByteSize));
            currentPage->m_next = newPage;
        }

        m_currentMarker.m_page = currentPage->m_next;
        m_currentMarker.m_offset = sizeof(Page);
    }


    LinearAllocator::LinearAllocator(const size_t pageByteSize, std::pmr::memory_resource* pageAllocator)
        : m_pageByteSize(pageByteSize)
        , m_pageAllocator(pageAllocator)
    {
        FE_Assert(pageByteSize > 0, "Page size must be greater than zero");
        if (m_pageAllocator == nullptr)
        {
            m_pageAllocator = std::pmr::get_default_resource();
        }
    }


    LinearAllocator::~LinearAllocator()
    {
        Destroy();
        Invalidate();
    }


    void LinearAllocator::FreeUnusedMemory()
    {
        Page* page = m_currentMarker.m_page;
        if (page == nullptr)
            return;

        page = page->m_next;
        m_currentMarker.m_page->m_next = nullptr;

        while (page)
        {
            Page* oldPage = page;
            page = page->m_next;
            m_pageAllocator->deallocate(oldPage, m_pageByteSize);
        }
    }
} // namespace FE::Memory
