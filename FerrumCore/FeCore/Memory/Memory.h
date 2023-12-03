#pragma once
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/HeapAllocator.h>
#include <FeCore/Memory/SharedPtr.h>
#include <FeCore/Memory/UniquePtr.h>
#include <deque>
#include <memory>
#include <unordered_map>

namespace FE
{
    //! \brief Create a `Unique<T>`.
    //!
    //! \param [in] args   - Arguments to call constructor of T with.
    //! \tparam T          - Type of object to allocate.
    //! \tparam TAllocator - Type of allocator to use for allocation and deallocation of the object.
    //! \tparam Args       - Types of arguments to call constructor of T with.
    //!
    //! \return An instance of `Unique<T>` that holds the allocated object of type T.
    template<class T, class TAllocator, class... Args>
    inline Unique<T, TAllocator> AllocateUnique(Args&&... args)
    {
        FE_STATIC_SRCPOS(position);
        void* data = FE::GlobalAllocator<TAllocator>::Get().Allocate(sizeof(T), alignof(T), position);
        T* obj     = new (data) T(std::forward<Args>(args)...);
        return Unique<T, TAllocator>(obj);
    }

    //! \brief Create a `Unique<T>` that uses \ref HeapAllocator.
    //!
    //! \param [in] args - Arguments to call constructor of T with.
    //! \tparam T        - Type of object to allocate.
    //! \tparam Args     - Types of arguments to call constructor of T with.
    //!
    //! \return An instance of `Unique<T>` that holds the allocated object of type T.
    template<class T, class... Args>
    inline Unique<T> MakeUnique(Args&&... args)
    {
        return AllocateUnique<T, HeapAllocator>(std::forward<Args>(args)...);
    }

    //! \brief Create a \ref Rc<T>.
    //!
    //! This function allocates storage for \ref ReferenceCounter and an object of type T.
    //! It attaches reference counter to the allocated object and returns an instance of \ref Rc<T>.
    //!
    //! \param [in] args   - Arguments to call constructor of T with.
    //! \tparam T          - Type of object to allocate.
    //! \tparam TAllocator - Type of allocator to use for allocation and deallocation of the object.
    //! \tparam Args       - Types of arguments to call constructor of T with.
    //!
    //! \return An instance of \ref RefCountPtr that holds the allocated object of type T.
    template<class T, class TAllocator, class... Args>
    inline Rc<T> AllocateShared(Args&&... args)
    {
        USize counterSize = AlignUp(sizeof(ReferenceCounter), alignof(T));
        USize wholeSize   = sizeof(T) + counterSize;

        IAllocator* allocator = &FE::GlobalAllocator<TAllocator>::Get();
        UInt8* ptr            = static_cast<UInt8*>(allocator->Allocate(wholeSize, alignof(T), FE_SRCPOS()));
        auto* counter         = new (ptr) ReferenceCounter(allocator);

        T* object = new (ptr + counterSize) T(std::forward<Args>(args)...);
        object->AttachRefCounter(counter);
        return Rc<T>(object);
    }

    //! \brief Create a \ref Rc<T>.
    //!
    //! This function allocates storage for \ref ReferenceCounter and an object of type T.
    //! It attaches reference counter to the allocated object and returns an instance of \ref Rc<T>.
    //!
    //! \param [in] args   - Arguments to call constructor of T with.
    //! \tparam T          - Type of object to allocate.
    //! \tparam Args       - Types of arguments to call constructor of T with.
    //!
    //! \return An instance of \ref RefCountPtr that holds the allocated object of type T.
    template<class T, class... Args>
    inline Rc<T> MakeShared(Args&&... args)
    {
        return AllocateShared<T, FE::HeapAllocator>(std::forward<Args>(args)...);
    }

    //! \brief Perform `static_cast` of \ref Rc<T>.
    //!
    //! This function retrieves a raw pointer using \ref Rc::Get() and does a static_cast to TDest.
    //! The result pointer is then used to create a new \ref Rc<T>.\n
    //! It can be used to cast a derived class to base.
    //!
    //! \note To cast a base class to derived, use \ref fe_dynamic_cast.
    //!
    //! \param [in] src - Source pointer.
    //! \tparam TDest   - The type of result pointer.
    //! \tparam TSrc    - The type of source pointer.
    //!
    //! \return An instance of \ref Rc<TDest> that holds the same object but statically casted.
    template<class TDest, class TSrc>
    inline Rc<TDest> static_pointer_cast(const Rc<TSrc>& src)
    {
        return Rc<TDest>(static_cast<TDest*>(src.Get()));
    }

    //! \brief Perform `static_cast` of \ref Unique<T>.
    //!
    //! This function retrieves a raw pointer using \ref Unique::Detach() and does a static_cast to TDest.
    //! The result pointer is then used to create a new \ref Unique<TDest>.\n
    //! It can be used to cast a derived class to base.
    //!
    //! \note To cast a base class to derived, use \ref fe_dynamic_cast.
    //!
    //! \param [in] src - Source pointer.
    //! \tparam TDest   - The type of result pointer.
    //! \tparam TSrc    - The type of source pointer.
    //!
    //! \return An instance of \ref Unique<TDest> that holds the same object but statically casted.
    template<class TDest, class TSrc>
    inline Unique<TDest> static_pointer_cast(Unique<TSrc>&& src)
    {
        return Unique<TDest>(static_cast<TDest*>(src.Detach()));
    }

    //! \brief Perform \ref fe_dynamic_cast of \ref Rc<T>.
    //!
    //! This function retrieves a raw pointer using \ref Rc::Get() and does a fe_dynamic_cast to TDest.
    //! The result pointer is then used to create a new \ref Rc<T>.\n
    //! It can be used to cast a base class to derived.
    //!
    //! \note To cast a derived class to base, use `static_cast`.
    //!
    //! \param [in] src - Source pointer.
    //! \tparam TDest   - The type of result pointer.
    //! \tparam TSrc    - The type of source pointer.
    //!
    //! \return An instance of \ref Rc<TDest> that holds the same object but dynamically casted.
    template<class TDest, class TSrc>
    inline Rc<TDest> dynamic_pointer_cast(const Rc<TSrc>& src)
    {
        return Rc<TDest>(fe_dynamic_cast<TDest*>(src.Get()));
    }

    //! \brief Perform \ref fe_dynamic_cast of \ref Unique<T>.
    //!
    //! This function retrieves a raw pointer using \ref Unique::Detach() and does a fe_dynamic_cast to TDest.
    //! The result pointer is then used to create a new \ref Unique<TDest>.\n
    //! It can be used to cast a base class to derived.
    //!
    //! \note To cast a derived class to base, use `static_cast`.
    //!
    //! \param [in] src - Source pointer.
    //! \tparam TDest   - The type of result pointer.
    //! \tparam TSrc    - The type of source pointer.
    //!
    //! \return An instance of \ref Unique<TDest> that holds the same object but dynamically casted.
    template<class TDest, class TSrc>
    inline Unique<TDest> dynamic_pointer_cast(Unique<TSrc>&& src)
    {
        return Unique<TDest>(fe_dynamic_cast<TDest*>(src.Detach()));
    }

    //! \brief Perform \ref fe_assert_cast of \ref Rc<T>.
    //!
    //! This function retrieves a raw pointer using \ref Rc::Get() and does a fe_assert_cast to TDest.
    //! The result pointer is then used to create a new \ref Rc<T>.\n
    //! It can be used to cast a base class to derived.
    //!
    //! \note You must be sure that dynamic cast from TSrc to TDest will succeed.
    //!
    //! \param [in] src - Source pointer.
    //! \tparam TDest   - The type of result pointer.
    //! \tparam TSrc    - The type of source pointer.
    //!
    //! \return An instance of \ref Rc<TDest> that holds the same object but dynamically casted.
    template<class TDest, class TSrc>
    inline Rc<TDest> assert_pointer_cast(const Rc<TSrc>& src)
    {
        return Rc<TDest>(fe_assert_cast<TDest*>(src.Get()));
    }

    //! \brief Perform \ref fe_assert_cast of \ref Unique<T>.
    //!
    //! This function retrieves a raw pointer using \ref Unique::Detach() and does a fe_assert_cast to TDest.
    //! The result pointer is then used to create a new \ref Unique<TDest>.\n
    //! It can be used to cast a base class to derived.
    //!
    //! \note You must be sure that dynamic cast from TSrc to TDest will succeed.
    //!
    //! \param [in] src - Source pointer.
    //! \tparam TDest   - The type of result pointer.
    //! \tparam TSrc    - The type of source pointer.
    //!
    //! \return An instance of \ref Unique<TDest> that holds the same object but dynamically casted.
    template<class TDest, class TSrc>
    inline Unique<TDest> assert_pointer_cast(Unique<TSrc>&& src)
    {
        return Unique<TDest>(fe_assert_cast<TDest*>(src.Detach()));
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
        inline explicit StdAllocator(const StdAllocator<TOther, TAlloc>& other) noexcept
            : m_Instance(other.GetImpl())
        {
        }

        inline StdAllocator() noexcept
        {
            m_Instance = &FE::GlobalAllocator<TAlloc>::Get();
        }

        inline explicit StdAllocator(TAlloc* instance) noexcept
        {
            m_Instance = instance;
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
            m_Instance->Deallocate(ptr, position, n * sizeof(T));
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

    //! \brief An alias for `std::wstring` that uses \ref HeapAllocator through \ref StdHeapAllocator.
    using WString = std::basic_string<wchar_t, std::char_traits<wchar_t>, StdHeapAllocator<wchar_t>>;

    //! \brief An alias for `std::vector` that uses \ref HeapAllocator through \ref StdHeapAllocator.
    template<class T>
    using Vector = std::vector<T, StdHeapAllocator<T>>;

    //! \brief An alias for `std::deque` that uses \ref HeapAllocator through \ref StdHeapAllocator.
    template<class T>
    using Deque = std::deque<T, StdHeapAllocator<T>>;

    //! \brief An alias for `std::list` that uses \ref HeapAllocator through \ref StdHeapAllocator.
    template<class T>
    using LinkedList = std::list<T, StdHeapAllocator<T>>;

    //! \brief An alias for `std::unordered_map` that uses \ref HeapAllocator through \ref StdHeapAllocator.
    template<class TKey, class TValue, class THasher = std::hash<TKey>, class TEq = std::equal_to<TKey>>
    using UnorderedMap = std::unordered_map<TKey, TValue, THasher, TEq, StdHeapAllocator<std::pair<const TKey, TValue>>>;
} // namespace FE
