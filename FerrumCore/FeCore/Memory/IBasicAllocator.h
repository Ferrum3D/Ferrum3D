#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/RTTI/RTTI.h>

namespace FE
{
    //! @brief Basic allocator, provides interface similar to `malloc()` and `free()`.
    //!
    //! This interface is used to allocate global environment and other allocators.
    class IBasicAllocator
    {
    public:
        FE_CLASS_RTTI(IBasicAllocator, "1D908F99-58A1-451D-A073-5E4226D59C80");

        //! \brief Allocate a block of memory.
        //!
        //! \param size      - Size of block to allocate.
        //! \param alignment - Alignment of block to allocate.
        //!
        //! \return The pointer to the allocated block.
        virtual void* Allocate(size_t size, size_t alignment) = 0;

        //! \brief Deallocate memory at pointer.
        //! \param ptr - Pointer returned by \ref IBasicAllocator::Allocate function.
        virtual void Deallocate(void* ptr) = 0;
    };

    //! @brief Wrapper on \ref IBasicAllocator compatible with `std::allocator`.
    //!
    //! @tparam T Type of values to allocate.
    template<class T>
    class StdBasicAllocator
    {
        IBasicAllocator* m_Instance;

    public:
        FE_CLASS_RTTI(StdBasicAllocator, "8077ABC7-F1F4-46D1-A4F0-626F2958BF26");

        using value_type = T;

        inline StdBasicAllocator(const StdBasicAllocator& other) noexcept
            : m_Instance(other.m_Instance)
        {
        }

        template<class U>
        inline StdBasicAllocator(const StdBasicAllocator<U>& other) noexcept
            : m_Instance(other.get_handle())
        {
        }

        inline StdBasicAllocator(IBasicAllocator* instance) noexcept
            : m_Instance(instance)
        {
        }

        inline IBasicAllocator* get_handle() const noexcept
        {
            return m_Instance;
        }

        inline value_type* allocate(size_t n) noexcept
        {
            return static_cast<value_type*>(m_Instance->Allocate(n * sizeof(T), alignof(T)));
        }

        inline void deallocate(value_type* ptr, [[maybe_unused]] size_t n) noexcept
        {
            m_Instance->Deallocate(ptr);
        }
    };

    template<class T, class U>
    inline bool operator==(const StdBasicAllocator<T>&, const StdBasicAllocator<U>&) noexcept
    {
        return true;
    }

    template<class T, class U>
    inline bool operator!=(const StdBasicAllocator<T>& x, const StdBasicAllocator<U>& y) noexcept
    {
        return !(x == y);
    }
} // namespace FE
