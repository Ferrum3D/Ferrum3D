#pragma once
#include <Memory/Allocator.h>
#include <Memory/HeapAllocator.h>

namespace FE
{
    namespace Internal
    {
        template<class T>
        class RefCountObject
        {
            FE::IAllocator* m_Allocator;
            std::aligned_storage_t<sizeof(T), alignof(T)> m_Storage;
            uint8_t m_RefCount;

        public:
            inline RefCountObject(FE::IAllocator* allocator)
                : m_Allocator(allocator)
                , m_RefCount(0)
            {
            }

            template<class... Args>
            inline void Construct(Args&&... args)
            {
                new (&m_Storage) T(std::forward<Args>(args)...);
            }

            inline void Destruct()
            {
                reinterpret_cast<T*>(&m_Storage)->~T();
            }

            inline T* Get()
            {
                return reinterpret_cast<T*>(&m_Storage);
            }

            inline void AddRef()
            {
                ++m_RefCount;
            }

            inline void Release()
            {
                if (--m_RefCount == 0)
                {
                    reinterpret_cast<T*>(&m_Storage)->~T();
                    FE_STATIC_SRCPOS(position);
                    m_Allocator->Deallocate(this, position, sizeof(*this));
                }
            }
        };
    } // namespace Internal

    template<class T>
    class RefCountPtr
    {
        Internal::RefCountObject<T>* m_Data;

    public:
        inline RefCountPtr() noexcept
            : m_Data(nullptr)
        {
        }

        inline RefCountPtr(Internal::RefCountObject<T>* storage) noexcept
            : m_Data(storage)
        {
            m_Data->AddRef();
        }

        inline RefCountPtr(const RefCountPtr& other) noexcept
            : m_Data(other.m_Data)
        {
            m_Data->AddRef();
        }

        inline RefCountPtr(RefCountPtr&& other) noexcept
            : m_Data(other.m_Data)
        {
            other.m_Data = nullptr;
        }

        inline void Swap(RefCountPtr& other)
        {
            auto* t = other.m_Data;
            other.m_Data = m_Data;
            m_Data = t;
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
            if (m_Data)
                m_Data->Release();
        }

        inline T& operator*()
        {
            return *m_Data->Get();
        }

        inline T* operator->()
        {
            return m_Data->Get();
        }

        inline const T& operator*() const
        {
            return *m_Data->Get();
        }

        inline const T* operator->() const
        {
            return m_Data->Get();
        }

        inline T* GetRaw()
        {
            return m_Data->Get();
        }

        inline const T* GetRaw() const
        {
            return m_Data->Get();
        }

        inline operator bool() const
        {
            return m_Data;
        }
    };

    template<class T>
    inline bool operator==(const RefCountPtr<T>& lhs, const RefCountPtr<T>& rhs)
    {
        return lhs.GetRaw() == rhs.GetRaw();
    }

    template<class T>
    inline bool operator!=(const RefCountPtr<T>& lhs, const RefCountPtr<T>& rhs)
    {
        return !(lhs == rhs);
    }
} // namespace FE
