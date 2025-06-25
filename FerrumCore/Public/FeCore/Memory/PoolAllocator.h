#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::Memory
{
    struct PoolAllocator : public std::pmr::memory_resource
    {
        PoolAllocator() = default;

        PoolAllocator(const char* name, const size_t elementByteSize, const uint32_t pageByteSize = 64 * 1024)
        {
            Initialize(name, elementByteSize, pageByteSize);
        }

        ~PoolAllocator() override
        {
            FreePages();
        }

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;
        PoolAllocator(PoolAllocator&&) = delete;
        PoolAllocator& operator=(PoolAllocator&&) = delete;

        void Initialize(const char* name, const size_t elementByteSize, const uint32_t pageByteSize = 64 * 1024)
        {
            FE_CoreAssert(m_elementByteSize == 0, "Pool already initialized");
            FE_CoreAssert(elementByteSize > 0);
            m_name = name;

#if FE_DEBUG
            const PlatformSpec platformSpec = GetPlatformSpec();
            FE_CoreAssert(AlignUp(pageByteSize, platformSpec.m_granularity) == pageByteSize,
                          "Page size must be aligned to virtual allocation granularity");
#endif

            m_elementByteSize = AlignUp<kDefaultAlignment>(elementByteSize);
            FE_CoreAssert(pageByteSize > m_elementByteSize);

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

        void Reinitialize(const size_t elementByteSize, const uint32_t pageByteSize)
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

        inline void* AllocateFromPage(Page* page) const;

        void FreePages()
        {
            FE_CoreAssertDebug(m_allocationCount == 0, "Leak detected");

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
        void do_deallocate(void* ptr, size_t byteSize, size_t byteAlignment) override;
        [[nodiscard]] bool do_is_equal(const memory_resource& other) const noexcept override
        {
            return this == &other;
        }
    };


    template<class T>
    struct Pool final : private PoolAllocator
    {
        Pool() = default;

        explicit Pool(const char* name, const uint32_t pageByteSize = 64 * 1024)
            : PoolAllocator(name, sizeof(T), pageByteSize)
        {
        }

        void Initialize(const char* name, const uint32_t pageByteSize = 64 * 1024)
        {
            PoolAllocator::Initialize(name, sizeof(T), pageByteSize);
        }

        using PoolAllocator::Deinitialize;

        [[nodiscard]] PoolAllocator* GetAllocator()
        {
            return this;
        }

        void Reinitialize(const uint32_t pageByteSize = 64 * 1024)
        {
            PoolAllocator::Reinitialize(sizeof(T), pageByteSize);
        }

        template<class... TArgs>
        T* New(TArgs&&... args)
        {
            return Memory::New<T>(GetAllocator(), std::forward<TArgs>(args)...);
        }

        void Delete(const T* ptr)
        {
            Memory::Delete<T>(GetAllocator(), const_cast<T*>(ptr), sizeof(T));
        }
    };


    template<class TLock>
    using LockedPoolAllocator = LockedMemoryResource<PoolAllocator, TLock>;

    using SpinLockedPoolAllocator = SpinLockedMemoryResource<PoolAllocator>;
} // namespace FE::Memory
