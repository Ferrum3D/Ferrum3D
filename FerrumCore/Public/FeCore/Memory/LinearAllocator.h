#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Threading/SpinLock.h>

namespace FE::Memory
{
    class LinearAllocator : public std::pmr::memory_resource
    {
        struct Page final
        {
            Page* pNext = nullptr;
        };

    public:
        struct Marker final
        {
            friend class LinearAllocator;

        private:
            Page* m_page = nullptr;
            size_t m_offset = sizeof(Page);
        };

        struct Scope final
        {
            explicit Scope(LinearAllocator& allocator)
                : m_allocator(&allocator)
            {
                m_marker = allocator.GetMarker();
            }

            ~Scope()
            {
                m_allocator->Restore(m_marker);
            }

            Scope(const Scope&) = delete;
            Scope(Scope&&) = delete;
            Scope& operator=(const Scope&) = delete;
            Scope& operator=(Scope&&) = delete;

        private:
            LinearAllocator* m_allocator;
            Marker m_marker;
        };

    private:
        size_t m_pageByteSize = 0;
        std::pmr::memory_resource* m_pageAllocator = nullptr;
        Page* m_firstPage = nullptr;
        Marker m_currentMarker;

        void NewPage();

    protected:
        void do_deallocate(void*, size_t, size_t) override {}
        bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
        {
            return this == &other;
        }

    public:
        LinearAllocator(size_t pageByteSize = 64 * 1024, std::pmr::memory_resource* pPageAllocator = nullptr);
        ~LinearAllocator() override;

        void FreeUnusedMemory();

        void* do_allocate(size_t byteSize, size_t byteAlignment) override;

        [[nodiscard]] bool IsEmpty() const
        {
            return m_currentMarker.m_page == nullptr
                || m_currentMarker.m_offset == sizeof(Page) && m_currentMarker.m_page == m_firstPage;
        }

        [[nodiscard]] Marker GetMarker() const
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

        void FreeMemory()
        {
            Clear();
            FreeUnusedMemory();
        }
    };


    inline void* LinearAllocator::do_allocate(const size_t byteSize, const size_t byteAlignment)
    {
        if (AlignUp(sizeof(Page), byteAlignment) + byteSize > m_pageByteSize)
            return nullptr;

        if (!m_currentMarker.m_page)
            NewPage();

        const size_t newOffset = AlignUp(m_currentMarker.m_offset, byteAlignment) + byteSize;
        if (newOffset > m_pageByteSize)
            NewPage();

        m_currentMarker.m_offset = newOffset;
        return reinterpret_cast<uint8_t*>(m_currentMarker.m_page) + newOffset - byteSize;
    }

    using SpinLockedLinearAllocator = LockedMemoryResource<LinearAllocator, Threading::SpinLock>;
} // namespace FE::Memory
