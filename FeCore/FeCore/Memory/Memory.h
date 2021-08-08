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

    template<class T, class TAllocator, class... Args>
    inline RefCountPtr<T> AllocateShared(Args&&... args)
    {
        constexpr size_t counterSize = AlignUp(sizeof(ReferenceCounter), alignof(T));
        constexpr size_t wholeSize   = sizeof(T) + counterSize;

        IAllocator* allocator     = &FE::GlobalAllocator<TAllocator>::Get();
        UInt8* ptr                = static_cast<UInt8*>(allocator->Allocate(wholeSize, alignof(T), FE_SRCPOS()));
        ReferenceCounter* counter = new (ptr) ReferenceCounter(allocator);

        T* object = new (ptr + counterSize) T(std::forward<Args>(args)...);
        object->AttachRefCounter(counter);
        return RefCountPtr<T>(object);
    }

    template<class T, class... Args>
    inline RefCountPtr<T> MakeShared(Args&&... args)
    {
        return AllocateShared<T, FE::HeapAllocator>(std::forward<Args>(args)...);
    }

    template<class TDest, class TSrc>
    RefCountPtr<TDest> StaticPtrCast(const RefCountPtr<TSrc>& src)
    {
        return RefCountPtr<TDest>(static_cast<TDest*>(src.GetRaw()));
    }

    template<class T, class TAlloc>
    class StdAllocator
    {
        mutable IAllocator* m_Instance;

    public:
        FE_CLASS_RTTI(StdAllocator, "18778CFF-60EF-49FA-A560-46CE59C2BEFF");

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
