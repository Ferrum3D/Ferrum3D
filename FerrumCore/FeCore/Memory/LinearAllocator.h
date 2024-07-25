#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::Memory
{
    class LinearAllocator final : public std::pmr::memory_resource
    {
        struct Page final
        {
            Page* pNext = nullptr;
        };

    public:
        class Marker final
        {
            friend class LinearAllocator;

            Page* m_pPage = nullptr;
            size_t m_Offset = sizeof(Page);
        };

    private:
        size_t m_PageByteSize = 0;
        std::pmr::memory_resource* m_pPageAllocator = nullptr;
        Page* m_pFirstPage = nullptr;
        Marker m_CurrentMarker;

        void NewPage();

        void* do_allocate(size_t byteSize, size_t byteAlignment) override;
        void do_deallocate(void*, size_t, size_t) override {}
        bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
        {
            return this == &other;
        }

    public:
        LinearAllocator(size_t pageByteSize = 64 * 1024, std::pmr::memory_resource* pPageAllocator = nullptr);
        ~LinearAllocator();

        void Collect();

        inline Marker GetMarker() const
        {
            return m_CurrentMarker;
        }

        inline void Restore(const Marker& marker)
        {
            m_CurrentMarker = marker;
        }

        inline void Clear()
        {
            m_CurrentMarker = {};
        }
    };


    inline void* LinearAllocator::do_allocate(size_t byteSize, size_t byteAlignment)
    {
        if (AlignUp(sizeof(Page), byteAlignment) + byteSize > m_PageByteSize)
            return nullptr;

        if (!m_CurrentMarker.m_pPage)
            NewPage();

        const size_t newOffset = AlignUp(m_CurrentMarker.m_Offset, byteAlignment) + byteSize;
        if (newOffset > m_PageByteSize)
            NewPage();

        const size_t oldOffset = m_CurrentMarker.m_Offset;
        m_CurrentMarker.m_Offset = newOffset;

        return reinterpret_cast<uint8_t*>(m_CurrentMarker.m_pPage) + oldOffset;
    }
} // namespace FE::Memory
