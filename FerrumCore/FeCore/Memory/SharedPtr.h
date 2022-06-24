#pragma once
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/HeapAllocator.h>
#include <FeCore/Memory/Object.h>

namespace FE
{
    //! \brief Shared smart pointer implementation that uses engine's internal reference counting system.
    //!
    //! For more information about engine's reference counting system see \ref ReferenceCounter.
    //!
    //! \tparam T - Type of object to hold.
    //!
    //! \note T _must_ inherit from \ref IObject.
    //! \see ReferenceCounter
    template<class T>
    class Shared final
    {
        T* m_Object;

    public:
        FE_STRUCT_RTTI(Shared, "FFF4F2AF-4AAA-49F6-AE7A-D28ED39C794E");

        //! \brief Create a _null_ reference counted pointer.
        inline Shared() noexcept
            : m_Object(nullptr)
        {
        }

        //! \brief Create a pointer that points to the specified object.
        //!
        //! \param [in] object - The pointer to object.
        //!
        //! \note It is valid to use this constructor on a raw object since the object itself stores
        //!       The pointer to \ref ReferenceCounter instance.
        inline Shared(T* object) noexcept
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
        inline Shared(const Shared& other) noexcept
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
        inline Shared(Shared&& other) noexcept
            : m_Object(other.m_Object)
        {
            other.m_Object = nullptr;
        }

        //! \brief Swap raw pointers of two objects without incrementing and descrementing ref-counters.
        inline void Swap(Shared& other)
        {
            auto* t        = other.m_Object;
            other.m_Object = m_Object;
            m_Object       = t;
        }

        //! \brief Copy a pointer (adds a strong reference to underlying object).
        //!
        //! \param [in] other - Pointer to copy.
        inline Shared& operator=(const Shared& other)
        {
            Shared(other).Swap(*this);
            return *this;
        }

        //! \brief Move a pointer (doesn't add a strong reference to underlying object).
        //!
        //! \param [in] other - Pointer to move.
        inline Shared& operator=(Shared&& other) noexcept
        {
            Shared(std::move(other)).Swap(*this);
            return *this;
        }

        //! \brief Set a pointer to _null_.
        inline void Reset()
        {
            Shared{}.Swap(*this);
        }

        inline ~Shared()
        {
            if (m_Object)
            {
                m_Object->ReleaseStrongRef();
            }
        }

        //! \brief Forget object and don't free it automatically.
        //!
        //! Can be useful to send an object to managed program.
        inline T* Detach()
        {
            if (m_Object)
            {
                m_Object->AddStrongRef();
            }

            return m_Object;
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
    inline bool operator==(const Shared<T>& lhs, std::nullptr_t)
    {
        return lhs.GetRaw() == nullptr;
    }

    template<class T>
    inline bool operator!=(const Shared<T>& lhs, std::nullptr_t)
    {
        return !(lhs == nullptr);
    }

    template<class T>
    inline bool operator==(std::nullptr_t, const Shared<T>& rhs)
    {
        return rhs.GetRaw() == nullptr;
    }

    template<class T>
    inline bool operator!=(std::nullptr_t, const Shared<T>& rhs)
    {
        return !(nullptr == rhs);
    }

    template<class T1, class T2>
    inline bool operator==(const Shared<T1>& lhs, const Shared<T2>& rhs)
    {
        return static_cast<IObject*>(lhs.GetRaw()) == static_cast<IObject*>(rhs.GetRaw());
    }

    template<class T1, class T2>
    inline bool operator!=(const Shared<T1>& lhs, const Shared<T2>& rhs)
    {
        return !(lhs == rhs);
    }
} // namespace FE
