#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::Memory
{
    class PoolAllocator : public std::pmr::memory_resource
    {
        struct Page final
        {
            Page* pNext;
            void* pCurrent;
        };

        static_assert(sizeof(Page) == kDefaultAlignment);

        const char* m_Name = nullptr;
        size_t m_ElementByteSize = 0;
        size_t m_PageByteSize = 0;

        Page* m_pPageList = nullptr;
        void* m_pFreeList = nullptr;

#if FE_DEBUG
        // Used to ensure we don't destroy or reinitialize pools that are still in use.
        uint32_t m_AllocationCount = 0;
#endif

        inline Page* AllocatePage();

        inline void* AllocateFromPage(Page* pPage) const;

        inline void FreePages()
        {
            FE_CORE_ASSERT(m_AllocationCount == 0, "Leak detected");

            Page* pPage = m_pPageList;
            while (pPage)
            {
                Page* pNext = pPage->pNext;
                Memory::FreeVirtual(pPage, m_PageByteSize);
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

    public:
        inline PoolAllocator(const char* name, size_t elementByteSize, uint32_t pageByteSize)
        {
            Initialize(name, elementByteSize, pageByteSize);
        }

        inline ~PoolAllocator() override
        {
            FreePages();
        }

        inline void Initialize(const char* name, size_t elementByteSize, uint32_t pageByteSize)
        {
            FE_CORE_ASSERT(m_ElementByteSize == 0, "Pool already initialized");
            FE_CORE_ASSERT(elementByteSize > 0, "");
            m_Name = name;

            if (IsDebugBuild)
            {
                const PlatformSpec platformSpec = GetPlatformSpec();
                FE_CORE_ASSERT(AlignUp(pageByteSize, platformSpec.Granularity) == pageByteSize,
                               "Page size must be aligned to virtual allocation granularity");
            }

            m_ElementByteSize = AlignUp<kDefaultAlignment>(elementByteSize);
            FE_CORE_ASSERT(pageByteSize > m_ElementByteSize, "");

            m_PageByteSize = pageByteSize;
        }

        inline void Deinitialize()
        {
            FreePages();
            m_ElementByteSize = 0;
            m_PageByteSize = 0;
            m_pPageList = nullptr;
            m_pFreeList = nullptr;
        }

        inline void Reinitialize(size_t elementByteSize, uint32_t pageByteSize)
        {
            Deinitialize();
            Initialize(m_Name, elementByteSize, pageByteSize);
        }
    };
} // namespace FE::Memory
