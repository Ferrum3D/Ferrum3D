#pragma once
#include <Memory/Allocator.h>
#include <Memory/HeapAllocator.h>
#include <Memory/SharedPtr.h>
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
        using TObject = Internal::RefCountObject<T>;
        IAllocator* allocator = &FE::GlobalAllocator<FE::HeapAllocator>::Get();
        void* data = allocator->Allocate(sizeof(TObject), alignof(TObject), position);
        auto* obj = new (data) TObject(allocator);
        obj->Construct(std::forward<Args>(args)...);
        return RefCountPtr<T>(obj);
    }
} // namespace FE
