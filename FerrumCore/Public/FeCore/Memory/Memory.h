#pragma once
#include <FeCore/Memory/RefCount.h>

namespace FE
{
    namespace Memory
    {
        struct PlatformSpec final
        {
            size_t m_pageSize = 0;
            size_t m_granularity = 0;
        };


        [[nodiscard]] PlatformSpec GetPlatformSpec();


        [[nodiscard]] void* AllocateVirtual(size_t byteSize);
        void FreeVirtual(void* ptr, size_t byteSize);


        enum class ProtectFlags : uint32_t
        {
            kNone = 0,
            kReadOnly = 1,
            kReadWrite = 2,
        };

        void ProtectVirtual(void* ptr, size_t byteSize, ProtectFlags protection);


        [[nodiscard]] void* DefaultAllocate(size_t byteSize);
        [[nodiscard]] void* DefaultAllocate(size_t byteSize, size_t byteAlignment);
        [[nodiscard]] void* DefaultReallocate(void* ptr, size_t newSize);
        void DefaultFree(void* ptr);


        //! @brief Allocate an uninitialized array using the provided allocator.
        template<class T, class TAllocator>
        [[nodiscard]] inline T* AllocateArray(TAllocator* pAllocator, size_t elementCount, size_t byteAlignment = alignof(T))
        {
            return static_cast<T*>(pAllocator->allocate(elementCount * sizeof(T), byteAlignment));
        }


        //! @brief Allocate an uninitialized array using the default allocator.
        template<class T>
        [[nodiscard]] inline T* DefaultAllocateArray(size_t elementCount, size_t byteAlignment = alignof(T))
        {
            return static_cast<T*>(DefaultAllocate(elementCount * sizeof(T), byteAlignment));
        }


        //! @brief Create a new object of type T using the provided allocator.
        //!
        //! @param pAllocator The allocator to use.
        //! @param args       The arguments to call the constructor of T with.
        //!
        //! @tparam T           The type of the object to allocate.
        //! @tparam TAllocator  The type of the provided allocator.
        //!
        //! @return The allocated object.
        template<class T, class TAllocator, class... TArgs>
        [[nodiscard]] inline T* New(TAllocator* pAllocator, TArgs&&... args)
        {
            return new (pAllocator->allocate(sizeof(T), alignof(T))) T(festd::forward<TArgs>(args)...);
        }


        //! @brief Create a new object of type T using the default allocator.
        //!
        //! @param args The arguments to call the constructor of T with.
        //!
        //! @tparam T  The type of the object to allocate.
        //!
        //! @return The allocated object.
        template<class T, class... TArgs>
        [[nodiscard]] inline T* DefaultNew(TArgs&&... args)
        {
            return new (DefaultAllocate(sizeof(T), alignof(T))) T(festd::forward<TArgs>(args)...);
        }


        //! @brief Delete an object previously created via Memory::New().
        //!
        //! @param pAllocator    The allocator to use.
        //! @param pointer       The pointer to the object to delete previously returned by Memory::New().
        //! @param byteSize      The size of the object to delete.
        //! @param byteAlignment The alignment that was specified when Memory::New() was called.
        //!
        //! @tparam T           The type of the object to delete.
        //! @tparam TAllocator  The type of the provided allocator.
        template<class T, class TAllocator>
        inline void Delete(TAllocator* pAllocator, T* pointer, size_t byteSize = 0, size_t byteAlignment = kDefaultAlignment)
        {
            pointer->~T();
            pAllocator->deallocate(pointer, byteSize, byteAlignment);
        }


        //! @brief A memory resource synchronized using a lock.
        //!
        //! @tparam TBase Base memory resource.
        //! @tparam TLock Type of the lock to use.
        template<class TBase, class TLock>
        struct LockedMemoryResource final : public TBase
        {
            using TBase::TBase;

            void* do_allocate(const size_t byteSize, const size_t byteAlignment) override
            {
                std::lock_guard lk{ m_lock };
                return TBase::do_allocate(byteSize, byteAlignment);
            }

            void do_deallocate(void* ptr, const size_t byteSize, const size_t byteAlignment) override
            {
                std::lock_guard lk{ m_lock };
                TBase::do_deallocate(ptr, byteSize, byteAlignment);
            }

        private:
            TLock m_lock;
        };


        struct FixedBlockAllocator final : public std::pmr::memory_resource
        {
            FixedBlockAllocator(void* memory, const size_t size)
                : m_begin(static_cast<std::byte*>(memory))
                , m_end(m_begin + size)
            {
            }

        private:
            std::byte* m_begin = nullptr;
            std::byte* m_end = nullptr;

            void* do_allocate(const size_t byteSize, const size_t byteAlignment) override
            {
                std::byte* alignedBegin = AlignUpPtr(m_begin, byteAlignment);
                if (byteSize > static_cast<size_t>(m_end - alignedBegin))
                    return nullptr;

                void* ptr = alignedBegin;
                m_begin = alignedBegin + byteSize;
                return ptr;
            }

            void do_deallocate(void*, size_t, size_t) override {}

            [[nodiscard]] bool do_is_equal(const memory_resource& other) const noexcept override
            {
                return this == &other;
            }
        };


        struct TlsfAllocator final : public std::pmr::memory_resource
        {
            TlsfAllocator(void* memory, size_t size);
            ~TlsfAllocator() override;

        private:
            void* do_allocate(size_t byteSize, size_t byteAlignment) override;
            void do_deallocate(void* ptr, size_t byteSize, size_t byteAlignment) override;

            [[nodiscard]] bool do_is_equal(const memory_resource& other) const noexcept override
            {
                return this == &other;
            }

            void* m_impl = nullptr;
            size_t m_size = 0;
            void* m_ownedMemory = nullptr;
        };


        //! @brief Delete an object previously created via Memory::DefaultNew().
        //!
        //! @param pointer The pointer to the object to delete previously returned by Memory::DefaultNew().
        //!
        //! @tparam T  The type of the object to delete.
        template<class T>
        void DefaultDelete(T* pointer)
        {
            pointer->~T();
            DefaultFree(pointer);
        }


        template<class T>
        struct DefaultDeleter
        {
            constexpr DefaultDeleter() noexcept = default;

            template<class T2>
            DefaultDeleter(const DefaultDeleter<T2>&) noexcept
            {
            }

            void operator()(T* ptr) const noexcept
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
            DefaultDeleter(const DefaultDeleter<T2[]>&) noexcept
            {
            }

            template<class T2>
            std::enable_if_t<std::is_trivially_destructible_v<T>> operator()(T2* ptr) const noexcept
            {
                DefaultFree(ptr);
            }
        };


        template<class T, class TSizeType = size_t>
        class StdDefaultAllocator final
        {
        public:
            using value_type = T;
            using size_type = TSizeType;

            constexpr StdDefaultAllocator() noexcept {}
            constexpr StdDefaultAllocator(const StdDefaultAllocator&) noexcept {}

            template<class U>
            constexpr StdDefaultAllocator(const StdDefaultAllocator<U, TSizeType>&) noexcept
            {
            }

            [[nodiscard]] T* allocate(const size_t n) const
            {
                return static_cast<T*>(DefaultAllocate(n * sizeof(T)));
            }

            void deallocate(T* ptr, size_t) const
            {
                DefaultFree(ptr);
            }

            template<class U>
            bool operator==(const StdDefaultAllocator<U, TSizeType>&) noexcept
            {
                return true;
            }

            template<class U>
            bool operator!=(const StdDefaultAllocator<U, TSizeType>&) noexcept
            {
                return false;
            }
        };
    } // namespace Memory


    namespace festd
    {
        template<class T>
        using unique_ptr = std::unique_ptr<T, Memory::DefaultDeleter<T>>;

        template<class T, class... TArgs>
        [[nodiscard]] unique_ptr<T> make_unique(TArgs&&... args)
        {
            return unique_ptr<T>(Memory::DefaultNew<T>(festd::forward<TArgs>(args)...));
        }
    } // namespace festd


    //! @brief Perform `static_cast` of Rc<T>.
    //!
    //! This function retrieves a raw pointer using Rc::Get() and does a static_cast to TDest.
    //! The result pointer is then used to create a new Rc<T>.\n
    //! It can be used to cast a derived class to base.
    //!
    //! @note To cast a base class to derived, use dynamic_pointer_cast.
    //!
    //! @param src Source pointer.
    //!
    //! @tparam TDest  The type of result pointer.
    //! @tparam TSrc   The type of source pointer.
    //!
    //! @return An instance of Rc<TDest> that holds the same object but statically cast.
    template<class TDest, class TSrc>
    [[nodiscard]] inline Rc<TDest> static_pointer_cast(const Rc<TSrc>& src)
    {
        return Rc<TDest>(static_cast<TDest*>(src.Get()));
    }


    //! @brief Perform fe_dynamic_cast of Rc<T>.
    //!
    //! This function retrieves a raw pointer using Rc::Get() and does a fe_dynamic_cast to TDest.
    //! The result pointer is then used to create a new Rc<T>.\n
    //! It can be used to cast a base class to derived.
    //!
    //! @note To cast a derived class to base, use `static_cast`.
    //!
    //! @param src Source pointer.
    //!
    //! @tparam TDest  The type of result pointer.
    //! @tparam TSrc   The type of source pointer.
    //!
    //! @return An instance of Rc<TDest> that holds the same object but dynamically cast.
    template<class TDest, class TSrc>
    [[nodiscard]] inline Rc<TDest> dynamic_pointer_cast(const Rc<TSrc>& src)
    {
        return Rc<TDest>(fe_dynamic_cast<TDest*>(src.Get()));
    }


    //! @brief Perform fe_assert_cast of Rc<T>.
    //!
    //! This function retrieves a raw pointer using Rc::Get() and does a fe_assert_cast to TDest.
    //! The result pointer is then used to create a new Rc<T>.\n
    //! It can be used to cast a base class to derived.
    //!
    //! @param src Source pointer.
    //!
    //! @tparam TDest  The type of result pointer.
    //! @tparam TSrc   The type of source pointer.
    //!
    //! @return An instance of Rc<TDest> that holds the same object but dynamically cast.
    template<class TDest, class TSrc>
    [[nodiscard]] inline Rc<TDest> assert_pointer_cast(const Rc<TSrc>& src)
    {
        return Rc<TDest>(fe_assert_cast<TDest*>(src.Get()));
    }
} // namespace FE
