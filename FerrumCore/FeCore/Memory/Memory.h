#pragma once
#include <FeCore/Memory/NullableHandle.h>
#include <FeCore/Memory/RefCount.h>

namespace FE
{
    namespace Memory
    {
        struct PlatformSpec final
        {
            size_t PageSize = 0;
            size_t Granularity = 0;
        };


        PlatformSpec GetPlatformSpec();


        void* AllocateVirtual(size_t byteSize);
        void FreeVirtual(void* ptr, size_t byteSize);


        void* DefaultAllocate(size_t byteSize);
        void* DefaultAllocate(size_t byteSize, size_t byteAlignment);
        void* DefaultReallocate(void* ptr, size_t newSize);
        void DefaultFree(void* ptr);


        //! \brief Create a new object of type T using the provided allocator.
        //!
        //! \param pAllocator - The allocator to use.
        //! \param args       - The arguments to call the constructor of T with.
        //!
        //! \tparam T          - The type of the object to allocate.
        //! \tparam TAllocator - The type of the provided allocator.
        //!
        //! \return The allocated object.
        template<class T, class TAllocator, class... TArgs>
        inline T* New(TAllocator* pAllocator, TArgs&&... args)
        {
            return new (pAllocator->allocate(sizeof(T), alignof(T))) T(std::forward<TArgs>(args)...);
        }


        //! \brief Create a new object of type T using the default allocator.
        //!
        //! \param args - The arguments to call the constructor of T with.
        //!
        //! \tparam T - The type of the object to allocate.
        //!
        //! \return The allocated object.
        template<class T, class... TArgs>
        inline T* DefaultNew(TArgs&&... args)
        {
            return new (DefaultAllocate(sizeof(T), alignof(T))) T(std::forward<TArgs>(args)...);
        }


        //! \brief Delete an object previously created via Memory::New().
        //!
        //! \param pAllocator - The allocator to use.
        //! \param pointer    - The pointer to the object to delete previously returned by Memory::New().
        //! \param byteSize   - The size of the object to delete.
        //!
        //! \tparam T          - The type of the object to delete.
        //! \tparam TAllocator - The type of the provided allocator.
        template<class T, class TAllocator>
        inline void Delete(TAllocator* pAllocator, T* pointer, size_t byteSize)
        {
            pointer->~T();
            pAllocator->deallocate(pointer, byteSize, alignof(T));
        }


        //! \brief Delete an object previously created via Memory::DefaultNew().
        //!
        //! \param pointer  - The pointer to the object to delete previously returned by Memory::DefaultNew().
        //! \param byteSize - The size of the object to delete.
        //!
        //! \tparam T - The type of the object to delete.
        template<class T>
        inline void DefaultDelete(T* pointer)
        {
            pointer->~T();
            DefaultFree(pointer);
        }


        template<class T>
        struct DefaultDeleter final
        {
            constexpr DefaultDeleter() noexcept = default;

            template<class T2>
            inline DefaultDeleter(const DefaultDeleter<T2>&) noexcept
            {
            }

            inline void operator()(T* ptr) const noexcept
            {
                ptr->~T();
                DefaultFree(ptr);
            }
        };


        template<class T>
        struct DefaultDeleter<T[]>
        {
            constexpr DefaultDeleter() noexcept = default;

            template<class T2>
            inline DefaultDeleter(const DefaultDeleter<T2[]>&) noexcept
            {
            }

            template<class T2>
            inline void operator()(T2* ptr) const noexcept
            {
                DefaultFree(ptr);
            }
        };


        template<class T>
        using Unique = std::unique_ptr<T, Memory::DefaultDeleter<T>>;

        template<class T, class... TArgs>
        [[nodiscard]] Unique<T> MakeUnique(TArgs&&... args)
        {
            return Unique<T>(DefaultNew<T>(std::forward<TArgs>(args)...));
        }


        template<class T, class TSizeType = size_t>
        class StdDefaultAllocator final
        {
        public:
            using value_type = T;
            using size_type = TSizeType;

            constexpr StdDefaultAllocator() noexcept {}
            inline constexpr StdDefaultAllocator(const StdDefaultAllocator&) noexcept {}

            template<class U>
            inline constexpr StdDefaultAllocator(const StdDefaultAllocator<U, TSizeType>&) noexcept
            {
            }

            [[nodiscard]] inline T* allocate(size_t n) const
            {
                return static_cast<T*>(DefaultAllocate(n * sizeof(T)));
            }

            inline void deallocate(T* ptr, size_t) const
            {
                DefaultFree(ptr);
            }

            template<class U>
            inline bool operator==(const StdDefaultAllocator<U, TSizeType>&) noexcept
            {
                return true;
            }

            template<class U>
            inline bool operator!=(const StdDefaultAllocator<U, TSizeType>&) noexcept
            {
                return false;
            }
        };
    } // namespace Memory

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
} // namespace FE
