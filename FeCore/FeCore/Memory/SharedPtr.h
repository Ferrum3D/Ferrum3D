#pragma once
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/HeapAllocator.h>
#include <FeCore/Memory/Object.h>

namespace FE
{
    //! \brief Shared pointer implementation that uses engine's internal reference counting system.
    //!
    //! For more information about engine's reference counting system see \ref ReferenceCounter.
    //!
    //! \note T _must_ inherit from \ref IObject.
    //! \see ReferenceCounter
    template<class T>
    class RefCountPtr
    {
        T* m_Object;

    public:
        FE_CLASS_RTTI(RefCountPtr<T>, "FFF4F2AF-4AAA-49F6-AE7A-D28ED39C794E");

        //! \brief Create a _null_ reference counted pointer.
        inline RefCountPtr() noexcept
            : m_Object(nullptr)
        {
        }

        //! \brief Create a pointer that points to the specified object.
        //!
        //! \param [in] object - The pointer to object.
        //!
        //! \note It is valid to use this constructor on a raw object since the object itself stores
        //!       The pointer to \ref ReferenceCounter instance.
        inline RefCountPtr(T* object) noexcept
            : m_Object(object)
        {
            if (m_Object)
            {
                m_Object->AddStrongRef();
            }
        }

        //! \brief Copy a pointer (adds a strong reference to underlying object).
        //!
        //! \param [in] other - Pointer to copy.
        inline RefCountPtr(const RefCountPtr& other) noexcept
            : m_Object(other.m_Object)
        {
            if (m_Object)
            {
                m_Object->AddStrongRef();
            }
        }

        //! \brief Move a pointer (doesn't add a strong reference to underlying object).
        //!
        //! \param [in] other - Pointer to move.
        inline RefCountPtr(RefCountPtr&& other) noexcept
            : m_Object(other.m_Object)
        {
            other.m_Object = nullptr;
        }

        //! \brief Swap raw pointers of two objects without incrementing and descrementing ref-counters.
        inline void Swap(RefCountPtr& other)
        {
            auto* t        = other.m_Object;
            other.m_Object = m_Object;
            m_Object       = t;
        }

        //! \brief Copy a pointer (adds a strong reference to underlying object).
        //!
        //! \param [in] other - Pointer to copy.
        inline RefCountPtr& operator=(const RefCountPtr& other)
        {
            RefCountPtr(other).Swap(*this);
            return *this;
        }

        //! \brief Move a pointer (doesn't add a strong reference to underlying object).
        //!
        //! \param [in] other - Pointer to move.
        inline RefCountPtr& operator=(RefCountPtr&& other) noexcept
        {
            RefCountPtr(std::move(other)).Swap(*this);
            return *this;
        }

        //! \brief Set a pointer to _null_.
        inline void Reset()
        {
            RefCountPtr{}.Swap(*this);
        }

        inline ~RefCountPtr()
        {
            if (m_Object)
            {
                m_Object->ReleaseStrongRef();
            }
        }

        //! \brief Get underlying raw pointer.
        inline T* GetRaw() const
        {
            return m_Object;
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

        inline operator bool() const
        {
            return m_Object;
        }
    };

    template<class T>
    inline bool operator==(const RefCountPtr<T>& lhs, std::nullptr_t)
    {
        return lhs.GetRaw() == nullptr;
    }

    template<class T>
    inline bool operator!=(const RefCountPtr<T>& lhs, std::nullptr_t)
    {
        return !(lhs == nullptr);
    }

    template<class T>
    inline bool operator==(std::nullptr_t, const RefCountPtr<T>& rhs)
    {
        return rhs.GetRaw() == nullptr;
    }

    template<class T>
    inline bool operator!=(std::nullptr_t, const RefCountPtr<T>& rhs)
    {
        return !(nullptr == rhs);
    }

    template<class T1, class T2>
    inline bool operator==(const RefCountPtr<T1>& lhs, const RefCountPtr<T2>& rhs)
    {
        return lhs.GetRaw() == rhs.GetRaw();
    }

    template<class T1, class T2>
    inline bool operator!=(const RefCountPtr<T1>& lhs, const RefCountPtr<T2>& rhs)
    {
        return !(lhs == rhs);
    }
} // namespace FE
