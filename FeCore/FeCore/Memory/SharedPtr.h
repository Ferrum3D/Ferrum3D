#pragma once
#include <FeCore/Memory/Allocator.h>
#include <FeCore/Memory/HeapAllocator.h>

namespace FE
{
    namespace Internal
    {
        struct RefCounter
        {
            void* Ptr;
            UInt32 Count;
            FE::IAllocator* Allocator;
            bool MakeShared;

            inline RefCounter(void* ptr, FE::IAllocator* allocator, bool makeShared, UInt32 count = 1)
                : Ptr(ptr)
                , Count(count)
                , Allocator(allocator)
                , MakeShared(makeShared)
            {
            }
        };
    } // namespace Internal

    template<class T>
    class RefCountPtr
    {
        Internal::RefCounter* m_Data;

    public:
        inline RefCountPtr() noexcept
            : m_Data(nullptr)
        {
        }

        inline RefCountPtr(T* ptr) noexcept
        {
            IAllocator* allocator = &FE::GlobalAllocator<FE::HeapAllocator>::Get();
            size_t size           = sizeof(Internal::RefCounter);
            size_t align          = alignof(Internal::RefCounter);

            m_Data = new (allocator->Allocate(size, align, FE_SRCPOS())) Internal::RefCounter(ptr, allocator, false);
        }

        inline RefCountPtr(Internal::RefCounter* storage) noexcept
            : m_Data(storage)
        {
            ++m_Data->Count;
        }

        inline RefCountPtr(const RefCountPtr& other) noexcept
            : m_Data(other.m_Data)
        {
            ++m_Data->Count;
        }

        inline RefCountPtr(RefCountPtr&& other) noexcept
            : m_Data(other.m_Data)
        {
            other.m_Data = nullptr;
        }

        inline void Swap(RefCountPtr& other)
        {
            auto* t      = other.m_Data;
            other.m_Data = m_Data;
            m_Data       = t;
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
            if (m_Data == nullptr || --m_Data->Count)
                return;
            IAllocator* allocator = m_Data->Allocator;
            bool makeShared       = m_Data->MakeShared;
            // don't specify size here because T can be a different type since the pointer could have been casted
            reinterpret_cast<T*>(m_Data->Ptr)->~T();
            allocator->Deallocate(m_Data->Ptr, FE_SRCPOS());
            if (makeShared)
                return;
            allocator->Deallocate(m_Data, FE_SRCPOS());
        }

        inline T* GetRaw() const
        {
            return static_cast<T*>(m_Data->Ptr);
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

        inline Internal::RefCounter* GetImpl() const
        {
            return m_Data;
        }

        inline operator bool() const
        {
            return m_Data;
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
