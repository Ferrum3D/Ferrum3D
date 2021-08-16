#pragma once
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/HeapAllocator.h>
#include <FeCore/Memory/SharedPtr.h>
#include <memory>

namespace FE
{
    //! \breif Deleter for `std::unique_ptr` that uses \ref GlobalAllocator.
    //!
    //! \tparam T      - Type of object to delete.
    //! \tparam TAlloc - Type of allocator used to allocate the object.
    //!                  This type _must_ implement \ref IAllocator.
    template<class T, class TAlloc>
    constexpr auto ObjectDeleter = [](T* obj) {
        FE_STATIC_SRCPOS(position);
        FE::GlobalAllocator<TAlloc>::Get().Deallocate(obj, position);
    };

    //! \brief Type of \ref ObjectDeleter.
    template<class T, class TAlloc>
    using TObjectDeleter = decltype(ObjectDeleter<T, TAlloc>);

    //! \brief Create a `std::unique_ptr` with \ref ObjectDeleter that uses \ref HeapAllocator.
    //!
    //! \param [in] args - Arguments to call constructor of T with.
    //! \tparam T        - Type of object to allocate.
    //! \tparam Args     - Types of arguments to call constructor of T with.
    //!
    //! \return An instance of `std::unique_ptr` that holds the allocated object of type T.
    template<class T, class... Args>
    auto MakeUnique(Args&&... args) -> std::unique_ptr<T, TObjectDeleter<T, FE::HeapAllocator>>
    {
        FE_STATIC_SRCPOS(position);
        void* data = FE::GlobalAllocator<FE::HeapAllocator>::Get().Allocate(sizeof(T), alignof(T), position);
        T* obj     = new (data) T(std::forward<Args>(args)...);
        return std::unique_ptr<T, TObjectDeleter<T, FE::HeapAllocator>>(obj, ObjectDeleter<T, FE::HeapAllocator>);
    }

    //! \brief Create a \ref RefCountPtr.
    //!
    //! This function allocates storage for \ref ReferenceCounter and an object of type T.
    //! It attaches reference counter to the allocated object and returns an instance of \ref RefCountPtr.
    //!
    //! \param [in] args   - Arguments to call constructor of T with.
    //! \tparam T          - Type of object to allocate.
    //! \tparam IAllocator - Type of allocator to use for allocation and deallocation of the object.
    //! \tparam Args       - Types of arguments to call constructor of T with.
    //!
    //! \return An instance of \ref RefCountPtr that holds the allocated object of type T.
    template<class T, class TAllocator, class... Args>
    inline RefCountPtr<T> AllocateShared(Args&&... args)
    {
        constexpr size_t counterSize = AlignUp(sizeof(ReferenceCounter), alignof(T));
        constexpr size_t wholeSize   = sizeof(T) + counterSize;

        IAllocator* allocator     = &FE::GlobalAllocator<TAllocator>::Get();
        UInt8* ptr                = static_cast<UInt8*>(allocator->Allocate(wholeSize, alignof(T), FE_SRCPOS()));
        auto* counter = new (ptr) ReferenceCounter(allocator);

        T* object = new (ptr + counterSize) T(std::forward<Args>(args)...);
        object->AttachRefCounter(counter);
        return RefCountPtr<T>(object);
    }

    //! \brief Create a \ref RefCountPtr.
    //!
    //! This function allocates storage for \ref ReferenceCounter and an object of type T.
    //! It attaches reference counter to the allocated object and returns an instance of \ref RefCountPtr.
    //!
    //! \param [in] args   - Arguments to call constructor of T with.
    //! \tparam T          - Type of object to allocate.
    //! \tparam Args       - Types of arguments to call constructor of T with.
    //!
    //! \return An instance of \ref RefCountPtr that holds the allocated object of type T.
    template<class T, class... Args>
    inline RefCountPtr<T> MakeShared(Args&&... args)
    {
        return AllocateShared<T, FE::HeapAllocator>(std::forward<Args>(args)...);
    }

    //! \brief Perform static_cast of \ref RefCountPtr.
    //!
    //! This function retrieves a raw pointer using \ref RefCountPtr::GetRaw and does a static_cast to TDest.
    //! The result pointer is then used to create a new \ref RefCountPtr.\n
    //! It can be used to cast a derived cast to base.
    //!
    //! \note To cast a base class to derived, use \ref fe_dynamic_cast.
    //!
    //! \param [in] src - Source pointer.
    //! \tparam TDest   - The type of result pointer.
    //! \tparam TSrc    - The type of source pointer.
    //!
    //! \return An instance of \ref RefCountPtr that holds the same object but with different interface.
    template<class TDest, class TSrc>
    RefCountPtr<TDest> static_pointer_cast(const RefCountPtr<TSrc>& src)
    {
        return RefCountPtr<TDest>(static_cast<TDest*>(src.GetRaw()));
    }

    //! \brief A wrapper for \ref IAllocator compatible with `std::allocator`.
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

    //! \brief An alias for \ref StdAllocator that uses \ref HeapAllocator.
    template<class T>
    using StdHeapAllocator = StdAllocator<T, HeapAllocator>;

    //! \brief An alias for `std::vector` that uses \ref HeapAllocator through \ref StdHeapAllocator.
    template<class T>
    using Vector = std::vector<T, StdHeapAllocator<T>>;
} // namespace FE
