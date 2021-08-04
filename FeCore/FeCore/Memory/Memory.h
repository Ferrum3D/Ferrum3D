#pragma once
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/HeapAllocator.h>
#include <FeCore/Memory/SharedPtr.h>
#include <memory>

namespace FE
{
    template<class T, class TAlloc>
    constexpr auto ObjectDeleter = [](T* obj) {
        FE_STATIC_SRCPOS(position);
        FE::GlobalAllocator<TAlloc>::Get().Deallocate(obj, position);
    };

    template<class T, class TAlloc>
    using TObjectDeleter = decltype(ObjectDeleter<T, TAlloc>);

    template<class T, class... Args>
    auto MakeUnique(Args&&... args) -> std::unique_ptr<T, TObjectDeleter<T, FE::HeapAllocator>>
    {
        FE_STATIC_SRCPOS(position);
        void* data = FE::GlobalAllocator<FE::HeapAllocator>::Get().Allocate(sizeof(T), alignof(T), position);
        T* obj     = new (data) T(std::forward<Args>(args)...);
        return std::unique_ptr<T, TObjectDeleter<T, FE::HeapAllocator>>(obj, ObjectDeleter<T, FE::HeapAllocator>);
    }

    template<class T, class... Args>
    auto MakeShared(Args&&... args) -> RefCountPtr<T>
    {
        FE_STATIC_SRCPOS(position);
        IAllocator* allocator = &FE::GlobalAllocator<FE::HeapAllocator>::Get();
        size_t size = sizeof(Internal::RefCounter) + sizeof(T);
        size_t align = std::max(alignof(Internal::RefCounter), alignof(T));
        uint8_t* allocation = static_cast<uint8_t*>(allocator->Allocate(size, align, FE_SRCPOS()));
        auto* ptr = new (allocation) T(std::forward<Args>(args)...);
        auto* data = new (allocation + sizeof(T)) Internal::RefCounter(ptr, allocator, true, 0);
        return RefCountPtr<T>(data);
    }

    template<class TDest, class TSrc>
    RefCountPtr<TDest> StaticPtrCast(const RefCountPtr<TSrc>& src)
    {
        return RefCountPtr<TDest>(src.GetImpl());
    }

    template<class T, class TAlloc>
    class StdAllocator
    {
        mutable IAllocator* m_Instance;
    public:
        using value_type = T;

        inline StdAllocator(const StdAllocator& other) noexcept
            : m_Instance(other.m_Instance)
        {
        }

        template<class TOther>
        inline StdAllocator(const StdAllocator<TOther, TAlloc>& other) noexcept
            : m_Instance(other.GetImpl())
        {
        }

        inline StdAllocator() noexcept
        {
            m_Instance = &FE::GlobalAllocator<TAlloc>::Get();
        }

        inline IAllocator* GetImpl() const noexcept
        {
            return m_Instance;
        }

        inline value_type* allocate(size_t n) noexcept
        {
            FE_STATIC_SRCPOS(position);
            return static_cast<value_type*>(m_Instance->Allocate(n * sizeof(T), alignof(T), position));
        }

        inline void deallocate(value_type* ptr, size_t n) noexcept
        {
            FE_STATIC_SRCPOS(position);
            m_Instance->Deallocate(ptr, position, n);
        }
    };

    template<class T1, class T2, class TAlloc>
    inline bool operator==(const StdAllocator<T1, TAlloc>&, const StdAllocator<T2, TAlloc>&) noexcept
    {
        return true;
    }

    template<class T1, class T2, class TAlloc>
    inline bool operator!=(const StdAllocator<T1, TAlloc>& x, const StdAllocator<T2, TAlloc>& y) noexcept
    {
        return !(x == y);
    }

    template<class T>
    using StdHeapAllocator = StdAllocator<T, HeapAllocator>;

    template<class T>
    using Vector = std::vector<T, StdHeapAllocator<T>>;
} // namespace FE
