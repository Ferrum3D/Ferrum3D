#pragma once
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/HeapAllocator.h>

namespace FE
{
    //! \brief Unique smart pointer.
    //!
    //! This smart pointer doesn't allow multiple references to same object, will automatically delete
    //! the object it holds using \ref TAlloc global allocator. Unlike \ref Rc<T> this class
    //! works with any type, so there's no requirement that \ref T must inherit from \ref IObject.
    //!
    //! \tparam T      - Type of object to hold.
    //! \tparam TAlloc - Type of allocator to use to delete the object.
    template<class T, class TAlloc = HeapAllocator>
    class Unique final
    {
        T* m_Object;

    public:
        FE_STRUCT_RTTI(Unique, "F460EC10-BC73-4769-A1B6-6DA7C100895B");

        //! \brief Create a _null_ unique pointer.
        inline Unique() noexcept
            : m_Object(nullptr)
        {
        }

        //! \brief Create a pointer that points to the specified object.
        //!
        //! \param [in] object - The pointer to object.
        inline Unique(T* ptr) noexcept
            : m_Object(ptr)
        {
        }

        Unique(const Unique&) = delete;
        Unique& operator=(const Unique&) = delete;

        //! \brief Move a pointer.
        //!
        //! \param [in] other - Pointer to move.
        inline Unique(Unique&& other) noexcept
            : m_Object(other.m_Object)
        {
            other.m_Object = nullptr;
        }

        //! \brief Swap raw pointers of two unique pointers.
        inline void Swap(Unique& other) noexcept
        {
            auto* t        = other.m_Object;
            other.m_Object = m_Object;
            m_Object       = t;
        }

        //! \brief Move a pointer.
        //!
        //! \param [in] other - Pointer to move.
        inline Unique& operator=(Unique&& other) noexcept
        {
            Unique(std::move(other)).Swap(*this);
            return *this;
        }

        //! \brief Set a pointer to _null_.
        inline void Reset() noexcept
        {
            Unique{}.Swap(*this);
        }

        inline ~Unique()
        {
            if (m_Object)
            {
                FE_STATIC_SRCPOS(position);
                GlobalAllocator<TAlloc>::Get().Deallocate(m_Object, position);
            }
        }

        //! \brief Get underlying raw pointer.
        inline T* GetRaw() const noexcept
        {
            return m_Object;
        }

        inline T* Detach() noexcept
        {
            auto* t  = GetRaw();
            m_Object = nullptr;
            return t;
        }

        inline T& operator*()
        {
            return *GetRaw();
        }

        inline T* operator->()
        {
            return GetRaw();
        }

        inline const T& operator*() const
        {
            return *GetRaw();
        }

        inline const T* operator->() const
        {
            return GetRaw();
        }

        inline explicit operator bool() const
        {
            return m_Object;
        }
    };

    template<class T>
    inline bool operator==(const Unique<T>& lhs, std::nullptr_t)
    {
        return lhs.GetRaw() == nullptr;
    }

    template<class T>
    inline bool operator!=(const Unique<T>& lhs, std::nullptr_t)
    {
        return !(lhs == nullptr);
    }

    template<class T>
    inline bool operator==(std::nullptr_t, const Unique<T>& rhs)
    {
        return rhs.GetRaw() == nullptr;
    }

    template<class T>
    inline bool operator!=(std::nullptr_t, const Unique<T>& rhs)
    {
        return !(nullptr == rhs);
    }

    template<class T1, class T2>
    inline bool operator==(const Unique<T1>& lhs, const Unique<T2>& rhs)
    {
        return lhs.GetRaw() == rhs.GetRaw();
    }

    template<class T1, class T2>
    inline bool operator!=(const Unique<T1>& lhs, const Unique<T2>& rhs)
    {
        return !(lhs == rhs);
    }
} // namespace FE
