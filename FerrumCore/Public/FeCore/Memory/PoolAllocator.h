#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::Memory
{
    struct PoolAllocator : public std::pmr::memory_resource
    {
        PoolAllocator() = default;

        PoolAllocator(const char* name, const uint32_t elementByteSize, const uint32_t elementAlignment = kDefaultAlignment,
                      const uint32_t pageByteSize = 64 * 1024)
        {
            Initialize(name, elementByteSize, elementAlignment, pageByteSize);
        }

        ~PoolAllocator() override
        {
            FreePages();
        }

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;
        PoolAllocator(PoolAllocator&&) = delete;
        PoolAllocator& operator=(PoolAllocator&&) = delete;

        void Initialize(const char* name, const uint32_t elementByteSize, const uint32_t elementAlignment = kDefaultAlignment,
                        const uint32_t pageByteSize = 64 * 1024)
        {
            FE_Assert(m_elementByteSize == 0, "Pool already initialized");
            FE_Assert(elementByteSize > 0);
            FE_Assert(elementAlignment > 0);
            m_name = name;

#if FE_DEBUG
            const PlatformSpec platformSpec = GetPlatformSpec();
            FE_Assert(AlignUp(pageByteSize, platformSpec.m_granularity) == pageByteSize,
                          "Page size must be aligned to virtual allocation granularity");

            FE_Assert(Math::IsPowerOfTwo(elementAlignment));
#endif

            m_elementByteSize = AlignUp(elementByteSize, elementAlignment);
            m_elementAlignment = elementAlignment;
            m_pageByteSize = pageByteSize;

            FE_Assert(pageByteSize > m_elementByteSize);
        }

        void Deinitialize(const bool ignoreLeaks = false)
        {
            FreePages(ignoreLeaks);
            m_elementByteSize = 0;
            m_pageByteSize = 0;
            m_pageList = nullptr;
            m_freeList = nullptr;
        }

        void Reinitialize(const uint32_t elementByteSize, const uint32_t elementAlignment, const uint32_t pageByteSize)
        {
            Deinitialize();
            Initialize(m_name, elementByteSize, elementAlignment, pageByteSize);
        }

        [[nodiscard]] uint32_t GetElementByteSize() const
        {
            return m_elementByteSize;
        }

        [[nodiscard]] uint32_t GetElementAlignment() const
        {
            return m_elementAlignment;
        }

        [[nodiscard]] uint32_t GetPageByteSize() const
        {
            return m_pageByteSize;
        }

    private:
        struct Page final
        {
            Page* m_next;
            void* m_current;
        };

        static_assert(sizeof(Page) == kDefaultAlignment);

        const char* m_name = nullptr;
        uint32_t m_elementByteSize = 0;
        uint32_t m_elementAlignment = 0;
        uint32_t m_pageByteSize = 0;

#if FE_DEBUG
        // Used to ensure we don't destroy or reinitialize pools that are still in use.
        uint32_t m_allocationCount = 0;
#endif

        Page* m_pageList = nullptr;
        void* m_freeList = nullptr;

        inline Page* AllocatePage();

        inline void* AllocateFromPage(Page* page) const;

        void FreePages(const bool ignoreLeaks = false)
        {
            FE_AssertDebug(m_allocationCount == 0 || ignoreLeaks, "Leak detected");
            m_allocationCount = 0;

            Page* pPage = m_pageList;
            while (pPage)
            {
                Page* pNext = pPage->m_next;
                FreeVirtual(pPage, m_pageByteSize);
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


    template<class TLock>
    using LockedPoolAllocator = LockedMemoryResource<PoolAllocator, TLock>;

    using SpinLockedPoolAllocator = SpinLockedMemoryResource<PoolAllocator>;


    template<class T>
    struct Pool final : private PoolAllocator
    {
        Pool() = default;

        explicit Pool(const char* name, const uint32_t pageByteSize = 64 * 1024)
            : PoolAllocator(name, sizeof(T), alignof(T), pageByteSize)
        {
        }

        void Initialize(const char* name, const uint32_t pageByteSize = 64 * 1024)
        {
            PoolAllocator::Initialize(name, sizeof(T), alignof(T), pageByteSize);
        }

        using PoolAllocator::Deinitialize;

        [[nodiscard]] PoolAllocator* GetAllocator()
        {
            return this;
        }

        void Reinitialize(const uint32_t pageByteSize = 64 * 1024)
        {
            PoolAllocator::Reinitialize(sizeof(T), alignof(T), pageByteSize);
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


    template<class T, class TLock>
    struct LockedPool final
    {
        LockedPool() = default;

        explicit LockedPool(const char* name, const uint32_t pageByteSize = 64 * 1024)
            : m_allocator(name, sizeof(T), alignof(T), pageByteSize)
        {
        }

        void Initialize(const char* name, const uint32_t pageByteSize = 64 * 1024)
        {
            m_allocator.Initialize(name, sizeof(T), alignof(T), pageByteSize);
        }

        void Deinitialize()
        {
            m_allocator.Deinitialize();
        }

        [[nodiscard]] PoolAllocator* GetAllocator()
        {
            return &m_allocator;
        }

        void Reinitialize(const uint32_t pageByteSize = 64 * 1024)
        {
            m_allocator.Reinitialize(sizeof(T), alignof(T), pageByteSize);
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

    private:
        LockedMemoryResource<PoolAllocator, TLock> m_allocator;
    };


    template<class T>
    using SpinLockedPool = LockedPool<T, Threading::SpinLock>;
} // namespace FE::Memory
