#pragma once
#include <FeCore/Utils/CoreUtils.h>

namespace FE
{
    class IBasicAllocator
    {
    public:
        virtual void* Allocate(size_t size, size_t alignment) = 0;

        virtual void Deallocate(void* ptr) = 0;
    };

    template<class T>
    class StdBasicAllocator
    {
        IBasicAllocator* m_Instance;

    public:
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
