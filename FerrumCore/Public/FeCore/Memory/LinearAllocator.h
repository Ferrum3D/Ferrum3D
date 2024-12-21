#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::Memory
{
    class LinearAllocator : public std::pmr::memory_resource
    {
        struct Page final
        {
            Page* pNext = nullptr;
        };

    public:
        class Marker final
        {
            friend class LinearAllocator;

            Page* m_page = nullptr;
            size_t m_offset = sizeof(Page);
        };

    private:
        size_t m_pageByteSize = 0;
        std::pmr::memory_resource* m_pageAllocator = nullptr;
        Page* m_firstPage = nullptr;
        Marker m_currentMarker;

        void NewPage();

    protected:
        void* do_allocate(size_t byteSize, size_t byteAlignment) override;
        void do_deallocate(void*, size_t, size_t) override {}
        bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
        {
            return this == &other;
        }

    public:
        LinearAllocator(size_t pageByteSize = 64 * 1024, std::pmr::memory_resource* pPageAllocator = nullptr);
        ~LinearAllocator() override;

        void Collect();

        Marker GetMarker() const
        {
            return m_currentMarker;
        }

        void Restore(const Marker& marker)
        {
            m_currentMarker = marker;
        }

        void Clear()
        {
            m_currentMarker = {};
        }
    };


    inline void* LinearAllocator::do_allocate(size_t byteSize, size_t byteAlignment)
    {
        if (AlignUp(sizeof(Page), byteAlignment) + byteSize > m_pageByteSize)
            return nullptr;

        if (!m_currentMarker.m_page)
            NewPage();

        const size_t newOffset = AlignUp(m_currentMarker.m_offset, byteAlignment) + byteSize;
        if (newOffset > m_pageByteSize)
            NewPage();

        const size_t oldOffset = m_currentMarker.m_offset;
        m_currentMarker.m_offset = newOffset;

        return reinterpret_cast<uint8_t*>(m_currentMarker.m_page) + oldOffset;
    }
} // namespace FE::Memory
