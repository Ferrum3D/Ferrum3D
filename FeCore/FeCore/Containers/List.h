#pragma once
#include <FeCore/Memory/Memory.h>
#include <algorithm>

namespace FE
{
    template<class T>
    class List final
    {
        inline static constexpr USize Alignment = 16;

        inline static T* Allocate(USize n) noexcept
        {
            FE_STATIC_SRCPOS(position);
            return static_cast<T*>(GlobalAllocator<HeapAllocator>::Get().Allocate(n * sizeof(T), Alignment, position));
        }

        inline void VAllocate(USize n) noexcept
        {
            auto allocation = Allocate(n);
            m_Begin         = allocation;
            m_End           = allocation;
            m_EndCap        = m_Begin + n;
        }

        inline void ConstructAtEnd(USize n)
        {
            for (USize i = 0; i < n; ++i)
            {
                new (m_End++) T();
            }
        }

        inline void MoveConstructAtEnd(T&& x)
        {
            new (m_End++) T(std::move(x));
        }

        inline void ConstructAtEnd(USize n, const T& x)
        {
            for (USize i = 0; i < n; ++i)
            {
                new (m_End++) T(x);
            }
        }

        inline void DestructAtEnd(T* newEnd)
        {
            T* ptr = m_End;
            while (newEnd != ptr)
            {
                (--ptr)->~T();
            }
            m_End = newEnd;
        }

        inline static void Deallocate(T* c, USize s) noexcept
        {
            FE_STATIC_SRCPOS(position);
            GlobalAllocator<HeapAllocator>::Get().Deallocate(c, position, s);
        }

        inline void VDeallocate() noexcept
        {
            Deallocate(m_Begin, Capacity() * sizeof(T));
            m_Begin  = nullptr;
            m_End    = nullptr;
            m_EndCap = nullptr;
        }

        inline static void CopyData(T* dest, const T* src, size_t count) noexcept
        {
            memcpy(dest, src, count * sizeof(T));
        }

        [[nodiscard]] inline USize Recommend(USize newSize) const noexcept
        {
            return std::max(2 * Capacity(), newSize);
        }

        inline void AppendImpl(USize n, bool shrink = false)
        {
            auto newCap = Recommend(n + Size());
            if (m_EndCap - m_End >= n)
            {
                if (shrink)
                {
                    newCap = n;
                }
                else
                {
                    return;
                }
            }

            if (Empty())
            {
                VAllocate(newCap);
                return;
            }

            T* newBegin = Allocate(newCap);
            T* newEnd   = newBegin + Size();

            CopyData(newBegin, m_Begin, Size());
            VDeallocate();

            m_Begin  = newBegin;
            m_End    = newEnd;
            m_EndCap = newBegin + newCap;
        }

        T* m_Begin  = nullptr;
        T* m_End    = nullptr;
        T* m_EndCap = nullptr;

    public:
        FE_STRUCT_RTTI(List, "F478A740-263E-4274-A0CC-3789769262E2");

        inline List() = default;

        inline List(List&& other) noexcept
        {
            Swap(other);
        }

        inline List(const List& other)
        {
            Reserve(other.Size());
            for (const auto& v : other)
            {
                Push(v);
            }
        }

        inline List(USize n, const T& x)
        {
            VAllocate(n);
            ConstructAtEnd(n, x);
        }

        inline List(std::initializer_list<T> list)
        {
            Assign(list);
        }

        inline List& operator=(std::initializer_list<T> list)
        {
            Assign(list);
            return *this;
        }

        inline void Assign(USize n, const T& x)
        {
            VDeallocate();
            AppendImpl(n);
            ConstructAtEnd(n, x);
        }

        inline void Assign(std::initializer_list<T> list)
        {
            for (auto& v : list)
            {
                Push(v);
            }
        }

        inline T& Push(T&& x)
        {
            AppendImpl(1);
            MoveConstructAtEnd(std::move(x));
            return Back();
        }

        inline T& Push(const T& x)
        {
            AppendImpl(1);
            ConstructAtEnd(1, x);
            return Back();
        }

        inline void RemoveBack()
        {
            FE_CORE_ASSERT(!Empty(), "List was empty");
            DestructAtEnd(m_End - 1);
        }

        inline T Pop()
        {
            auto res = Back();
            RemoveBack();
            return res;
        }

        inline List& Append(const T& x)
        {
            Push(x);
            return *this;
        }

        inline List& Append(T&& x)
        {
            Push(std::move(x));
            return *this;
        }

        inline List& Append(USize n)
        {
            AppendImpl(n);
            ConstructAtEnd(n);
            return *this;
        }

        inline List& Append(USize n, const T& x)
        {
            AppendImpl(n);
            ConstructAtEnd(n, x);
            return *this;
        }

        template<class... Args>
        inline T& Emplace(Args&&... args)
        {
            AppendImpl(1);
            new (m_End++) T(std::forward<Args>(args)...);
            return Back();
        }

        inline ~List()
        {
            Clear();
            VDeallocate();
        }

        [[nodiscard]] inline USize Size() const noexcept
        {
            return m_End - m_Begin;
        }

        [[nodiscard]] inline USize Capacity() const noexcept
        {
            return m_EndCap - m_Begin;
        }

        [[nodiscard]] inline bool Empty() const noexcept
        {
            return m_End == m_Begin;
        }

        inline void Reserve(USize n)
        {
            if (Capacity() >= n)
            {
                return;
            }

            AppendImpl(n - Size());
        }

        inline void Resize(USize n)
        {
            Resize(n, T{});
        }

        inline void Resize(USize n, const T& x)
        {
            Reserve(n);
            if (Size() > n)
            {
                m_End = m_Begin + n;
                return;
            }

            ConstructAtEnd(n - Size(), x);
        }

        inline void Swap(List<T>& other)
        {
            std::swap(m_Begin, other.m_Begin);
            std::swap(m_End, other.m_End);
            std::swap(m_EndCap, other.m_EndCap);
        }

        inline void Shrink()
        {
            if (Capacity() == Size())
            {
                return;
            }

            if (Size() == 0)
            {
                VDeallocate();
                return;
            }

            AppendImpl(0, true);
        }

        inline void Sort()
        {
            std::sort(m_Begin, m_End);
        }

        [[nodiscard]] inline T& operator[](USize index) noexcept
        {
            FE_CORE_ASSERT(index < Size(), "Invalid index");
            return m_Begin[index];
        }

        [[nodiscard]] inline const T& operator[](USize index) const noexcept
        {
            FE_CORE_ASSERT(index < Size(), "Invalid index");
            return m_Begin[index];
        }

        inline void Clear()
        {
            DestructAtEnd(m_Begin);
        }

        [[nodiscard]] inline T& Front() noexcept
        {
            FE_CORE_ASSERT(!Empty(), "List was empty");
            return *m_Begin;
        }

        [[nodiscard]] inline const T& Front() const noexcept
        {
            FE_CORE_ASSERT(!Empty(), "List was empty");
            return *m_Begin;
        }

        [[nodiscard]] inline T& Back() noexcept
        {
            FE_CORE_ASSERT(!Empty(), "List was empty");
            return *(m_End - 1);
        }

        [[nodiscard]] inline const T& Back() const noexcept
        {
            FE_CORE_ASSERT(!Empty(), "List was empty");
            return *(m_End - 1);
        }

        [[nodiscard]] inline T* Data() noexcept
        {
            return m_Begin;
        }

        [[nodiscard]] inline const T* Data() const noexcept
        {
            return m_Begin;
        }

        inline T* begin() noexcept
        {
            return m_Begin;
        }

        inline const T* begin() const noexcept
        {
            return m_Begin;
        }

        inline T* end() noexcept
        {
            return m_End;
        }

        inline const T* end() const noexcept
        {
            return m_End;
        }
    };
} // namespace FE