#pragma once
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/HeapAllocator.h>
#include <FeCore/Memory/Object.h>

namespace FE
{
    template<class T, std::enable_if_t<std::is_base_of_v<IObject, T>, bool> = true>
    class RefCountPtr
    {
        T* m_Object;

    public:
        FE_CLASS_RTTI(RefCountPtr<T>, "FFF4F2AF-4AAA-49F6-AE7A-D28ED39C794E");

        inline RefCountPtr() noexcept
            : m_Object(nullptr)
        {
        }

        inline RefCountPtr(T* object) noexcept
            : m_Object(object)
        {
            m_Object->AddStrongRef();
        }

        inline RefCountPtr(const RefCountPtr& other) noexcept
            : m_Object(other.m_Object)
        {
            m_Object->AddStrongRef();
        }

        inline RefCountPtr(RefCountPtr&& other) noexcept
            : m_Object(other.m_Object)
        {
            other.m_Object = nullptr;
        }

        inline void Swap(RefCountPtr& other)
        {
            auto* t      = other.m_Object;
            other.m_Object = m_Object;
            m_Object       = t;
        }

        inline RefCountPtr& operator=(const RefCountPtr& other)
        {
            RefCountPtr(other).Swap(*this);
            return *this;
        }

        inline RefCountPtr& operator=(RefCountPtr&& other)
        {
            RefCountPtr(std::move(other)).Swap(*this);
            return *this;
        }

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
