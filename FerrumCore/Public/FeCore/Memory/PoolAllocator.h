#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::Memory
{
    struct PoolAllocator : public std::pmr::memory_resource
    {
        PoolAllocator(const char* name, size_t elementByteSize, uint32_t pageByteSize)
        {
            Initialize(name, elementByteSize, pageByteSize);
        }

        ~PoolAllocator() override
        {
            FreePages();
        }

        void Initialize(const char* name, size_t elementByteSize, uint32_t pageByteSize)
        {
            FE_CORE_ASSERT(m_elementByteSize == 0, "Pool already initialized");
            FE_CORE_ASSERT(elementByteSize > 0, "");
            m_name = name;

#if FE_DEBUG
            const PlatformSpec platformSpec = GetPlatformSpec();
            FE_CORE_ASSERT(AlignUp(pageByteSize, platformSpec.m_granularity) == pageByteSize,
                           "Page size must be aligned to virtual allocation granularity");
#endif

            m_elementByteSize = AlignUp<kDefaultAlignment>(elementByteSize);
            FE_CORE_ASSERT(pageByteSize > m_elementByteSize, "");

            m_pageByteSize = pageByteSize;
        }

        void Deinitialize()
        {
            FreePages();
            m_elementByteSize = 0;
            m_pageByteSize = 0;
            m_pageList = nullptr;
            m_freeList = nullptr;
        }

        void Reinitialize(size_t elementByteSize, uint32_t pageByteSize)
        {
            Deinitialize();
            Initialize(m_name, elementByteSize, pageByteSize);
        }

    private:
        struct Page final
        {
            Page* m_next;
            void* m_current;
        };

        static_assert(sizeof(Page) == kDefaultAlignment);

        const char* m_name = nullptr;
        size_t m_elementByteSize = 0;
        size_t m_pageByteSize = 0;

        Page* m_pageList = nullptr;
        void* m_freeList = nullptr;

#if FE_DEBUG
        // Used to ensure we don't destroy or reinitialize pools that are still in use.
        uint32_t m_allocationCount = 0;
#endif

        inline Page* AllocatePage();

        inline void* AllocateFromPage(Page* pPage) const;

        void FreePages()
        {
            FE_CORE_ASSERT(m_allocationCount == 0, "Leak detected");

            Page* pPage = m_pageList;
            while (pPage)
            {
                Page* pNext = pPage->m_next;
                Memory::FreeVirtual(pPage, m_pageByteSize);
                pPage = pNext;
            }
        }

    protected:
        void* do_allocate(size_t byteSize, size_t byteAlignment) override;
        void do_deallocate(void* ptr, size_t, size_t) override;
        bool do_is_equal(const memory_resource& other) const noexcept override
        {
            return this == &other;
        }
    };
} // namespace FE::Memory
