#pragma once
#include <FeCore/Memory/Memory.h>
#include <algorithm>
#include <tuple>

namespace FE
{
    //! \brief A List of elements stored contiguously in memory. Can grow when out of memory.
    //!
    //! Memory allocations in the list are always aligned to 16 bytes. It means that the pointer to the first element
    //! has 16-byte alignment, but the rest are stored tightly packed.
    template<class T>
    class List final
    {
        inline static constexpr USize Alignment = 16;

        template<USize I>
        [[nodiscard]] inline const T& GetAt() const
        {
            return m_Begin[I];
        }

        template<USize... I>
        [[nodiscard]] inline auto AsTupleImpl(std::index_sequence<I...>) const
        {
            return std::make_tuple(GetAt<I>()...);
        }

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
            if (c == nullptr)
            {
                return;
            }

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

        inline static void MoveData(T* dest, T* src, size_t count) noexcept
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                memcpy(dest, src, count * sizeof(T));
            }
            else
            {
                for (USize i = 0; i < count; ++i)
                {
                    new (&dest[i]) T(std::move(src[i]));
                }
            }
        }

        [[nodiscard]] inline USize Recommend(USize newSize) const noexcept
        {
            return std::max(2 * Capacity(), newSize);
        }

        inline void AppendImpl(USize n, bool shrink = false)
        {
            auto newCap = Recommend(n + Size());
            if (m_EndCap - m_End >= static_cast<SSize>(n))
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

            MoveData(newBegin, m_Begin, Size());
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

        inline List& operator=(List&& other) noexcept
        {
            Swap(other);
            return *this;
        }

        inline List& operator=(const List& other)
        {
            List lst(other);
            Swap(lst);
            return *this;
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

        //! \brief Assign N copies of X.
        //!
        //! \param [in] n - The length of data.
        //! \param [in] x - The element to copy N times.
        inline void Assign(USize n, const T& x)
        {
            Clear();
            AppendImpl(n);
            ConstructAtEnd(n, x);
        }

        //! \brief Assign data from a std::initializer_list.
        //!
        //! \param [in] list - The list to assign.
        inline void Assign(std::initializer_list<T> list)
        {
            for (auto& v : list)
            {
                Push(v);
            }
        }

        //! \brief Assign data from a C-style array.
        //!
        //! \param [in] begin - A pointer to the array.
        //! \param [in] end - A pointer to the element after array that won't be included.
        inline void Assign(const T* begin, const T* end)
        {
            Reserve(end - begin);
            for (; begin != end; ++begin)
            {
                Push(*begin);
            }
        }

        //! \brief Push a new element to the back of the container and move.
        //!
        //! \param [in] x - The element to push.
        //!
        //! \return The reference to the back of the container.
        inline T& Push(T&& x)
        {
            AppendImpl(1);
            MoveConstructAtEnd(std::move(x));
            return Back();
        }

        //! \brief Push a new element to the back of the container and copy.
        //!
        //! \param [in] x - The element to push.
        //!
        //! \return The reference to the back of the container.
        inline T& Push(const T& x)
        {
            AppendImpl(1);
            ConstructAtEnd(1, x);
            return Back();
        }

        //! \brief Destruct the back of the container.
        inline void RemoveBack()
        {
            FE_CORE_ASSERT(!Empty(), "List was empty");
            DestructAtEnd(m_End - 1);
        }

        //! \brief Pop an element from the back.
        inline T Pop()
        {
            auto res = Back();
            RemoveBack();
            return res;
        }

        //! \brief Append a new element to the back of the container and move.
        //!
        //! \param [in] x - The element to push.
        //!
        //! \return The reference to the container.
        inline List& Append(T&& x)
        {
            Push(std::move(x));
            return *this;
        }

        //! \brief Append a new element to the back of the container and copy.
        //!
        //! \param [in] x - The element to push.
        //!
        //! \return The reference to the container.
        inline List& Append(const T& x)
        {
            Push(x);
            return *this;
        }

        //! \brief Append N default constructed elements to the back of the container.
        //!
        //! \param [in] n - The number of elements to append.
        //!
        //! \return The reference to the container.
        inline List& Append(USize n)
        {
            AppendImpl(n);
            ConstructAtEnd(n);
            return *this;
        }

        //! \brief Append N copies of the specified element to the back of the container.
        //!
        //! \param [in] n - The number of elements to append.
        //! \param [in] x - The element to copy N times.
        //!
        //! \return The reference to the container.
        inline List& Append(USize n, const T& x)
        {
            AppendImpl(n);
            ConstructAtEnd(n, x);
            return *this;
        }

        //! \brief Append a C-style array to the back of the container.
        //!
        //! \param [in] n    - The number of elements to append.
        //! \param [in] data - The data to be appended.
        //!
        //! \return The reference to the container.
        inline List& Append(USize n, const T* data)
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                AppendImpl(n);
                memcpy(m_End, data, n);
                m_End += n;
            }
            else
            {
                Reserve(Size() + n);
                for (USize i = 0; i < n; ++i)
                {
                    Push(data[i]);
                }
            }

            return *this;
        }

        //! \brief Construct an element in place at back of the container.
        //!
        //! \tparam Args - Types of the arguments.
        //! \param args - The arguments to the element constructor with.
        //!
        //! \return The reference to the back of the container.
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

        //! \brief Get the length of the container in number of elements.
        [[nodiscard]] inline USize Size() const noexcept
        {
            return m_End - m_Begin;
        }

        //! \brief Get the capacity of the container.
        [[nodiscard]] inline USize Capacity() const noexcept
        {
            return m_EndCap - m_Begin;
        }

        //! \brief Check if the container is empty.
        [[nodiscard]] inline bool Empty() const noexcept
        {
            return m_End == m_Begin;
        }

        //! \brief Check if the container has any elements.
        [[nodiscard]] inline bool Any() const noexcept
        {
            return !Empty();
        }

        //! \brief Reserve capacity for N elements. Does nothing if Capacity() >= N.
        //!
        //! \param [in] n - The capacity to reserve.
        inline void Reserve(USize n)
        {
            if (Capacity() >= n)
            {
                return;
            }

            AppendImpl(n - Size());
        }

        //! \brief Resize the container.
        //!
        //! If Size() < n, will construct new elements on the back to fit size.
        //! If Size() > n, Will destruct to fit size.
        //!
        //! \param [in] n - The new size of the container.
        inline void Resize(USize n)
        {
            Resize(n, T{});
        }

        //! \brief Resize the container.
        //!
        //! If Size() < n, will construct new elements on the back to fit size.
        //! If Size() > n, Will destruct to fit size.
        //!
        //! \param [in] n - The new size of the container.
        //! \param [in] x - The element to copy to the new elements (if any were added).
        inline void Resize(USize n, const T& x)
        {
            auto size = Size();
            if (n == size)
            {
                return;
            }

            Reserve(n);
            if (size > n)
            {
                DestructAtEnd(m_Begin + n);
                return;
            }

            ConstructAtEnd(n - Size(), x);
        }

        //! \brief Find a value if the container and return its index or -1.
        //!
        //! \param [in] value - The value to find.
        inline SSize IndexOf(const T& value) const
        {
            auto size = static_cast<SSize>(Size());
            for (SSize i = 0; i < size; ++i)
            {
                if (m_Begin[i] == value)
                {
                    return i;
                }
            }

            return -1;
        }

        //! \brief Check if a value is present in the list.
        //!
        //! \param [in] value - The value to check for.
        inline bool Contains(const T& value) const
        {
            return IndexOf(value) != -1;
        }

        //! \brief Remove value at index, will move all right values by one to the left.
        inline void RemoveAt(USize index)
        {
            FE_CORE_ASSERT(index < Size(), "Invalid index");
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                memmove(m_Begin + index, m_Begin + index + 1, Size() - index - 1);
            }
            else
            {
                auto size = Size();
                for (USize i = index; i < size - 1; ++i)
                {
                    m_Begin[i] = m_Begin[i + 1];
                }
            }

            DestructAtEnd(m_End - 1);
        }

        //! \brief Remove value at index, will swap the last value with the removed value.
        inline void SwapRemoveAt(USize index)
        {
            FE_CORE_ASSERT(index < Size(), "Invalid index");
            if (index < Size() - 1)
            {
                std::swap(m_Begin[index], m_Begin[Size() - 1]);
            }

            DestructAtEnd(m_End - 1);
        }

        //! \brief Remove the value, will move all right values by one to the left.
        inline bool Remove(const T& value)
        {
            auto index = IndexOf(value);
            if (index == -1)
            {
                return false;
            }

            RemoveAt(static_cast<USize>(index));
            return true;
        }

        //! \brief Remove the value, will swap the last value with the removed value.
        inline bool SwapRemove(const T& value)
        {
            auto index = IndexOf(value);
            if (index == -1)
            {
                return false;
            }

            SwapRemoveAt(static_cast<USize>(index));
            return true;
        }

        //! \bried Swap two lists.
        inline void Swap(List<T>& other)
        {
            std::swap(m_Begin, other.m_Begin);
            std::swap(m_End, other.m_End);
            std::swap(m_EndCap, other.m_EndCap);
        }

        //! \brief Set the capacity to be equal to the size. Useful to free wasted memory.
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

        //! \brief Sort elements in the container using a member of T as sorting key.
        //!
        //! \tparam TMember - Type of pointer to member (must implement relational operators).
        //!
        //! \param [in] member     - Pointer to member to use as sorting key.
        //! \param [in] descending - True if the List must be sorted in descending order.
        template<class TMember>
        inline void SortByMember(TMember member,
                                 std::enable_if_t<std::is_member_object_pointer_v<TMember>, bool> descending = false)
        {
            if (descending)
            {
                Sort([member](const T& lhs, const T& rhs) {
                    return lhs.*member > rhs.*member;
                });
            }
            else
            {
                Sort([member](const T& lhs, const T& rhs) {
                    return lhs.*member < rhs.*member;
                });
            }
        }

        template<class TMember>
        inline void SortByMember(TMember member,
                                 std::enable_if_t<std::is_member_function_pointer_v<TMember>, bool> descending = false)
        {
            if (descending)
            {
                Sort([member](const T& lhs, const T& rhs) {
                    return std::invoke(member, lhs) > std::invoke(member, rhs);
                });
            }
            else
            {
                Sort([member](const T& lhs, const T& rhs) {
                    return std::invoke(member, lhs) < std::invoke(member, rhs);
                });
            }
        }

        //! \brief Sort elements in the container by predicate.
        //!
        //! Predicate signature:
        //! \code{.cpp}
        //!     bool pred(const T& left, const T& right);
        //! \endcode
        //!
        //! \tparam F - Type of the predicate.
        //!
        //! \param [in] f - A functor to use as the sorting predicate.
        template<class F>
        inline void Sort(F&& f)
        {
            std::sort(m_Begin, m_End, std::forward<F>(f));
        }

        //! \brief Sort elements in the container.
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

        //! \brief Clear the container.
        inline void Clear()
        {
            DestructAtEnd(m_Begin);
        }

        //! \brief Get the first element of the container.
        [[nodiscard]] inline T& Front() noexcept
        {
            FE_CORE_ASSERT(!Empty(), "List was empty");
            return *m_Begin;
        }

        //! \brief Get the first element of the container.
        [[nodiscard]] inline const T& Front() const noexcept
        {
            FE_CORE_ASSERT(!Empty(), "List was empty");
            return *m_Begin;
        }

        //! \brief Get the last element of the container.
        [[nodiscard]] inline T& Back() noexcept
        {
            FE_CORE_ASSERT(!Empty(), "List was empty");
            return *(m_End - 1);
        }

        //! \brief Get the last element of the container.
        [[nodiscard]] inline const T& Back() const noexcept
        {
            FE_CORE_ASSERT(!Empty(), "List was empty");
            return *(m_End - 1);
        }

        //! \brief Get the pointer to the first element of the container.
        [[nodiscard]] inline T* Data() noexcept
        {
            return m_Begin;
        }

        //! \brief Get the pointer to the first element of the container.
        [[nodiscard]] inline const T* Data() const noexcept
        {
            return m_Begin;
        }

        //! \brief Convert a list to a tuple that contains all of its elements. Asserts that `I == Size()`.
        template<USize I>
        [[nodiscard]] inline auto AsTuple() const
        {
            FE_CORE_ASSERT(I == Size(), "Tuple size must match List size");
            return AsTupleImpl(std::make_index_sequence<I>{});
        }

        //! \brief Get the pointer to the first element of the container and reset.
        //!
        //! \note The caller is responsible to deallocate the memory.
        [[nodiscard]] inline T* DetachData() noexcept
        {
            auto result = m_Begin;
            m_Begin     = nullptr;
            m_End       = nullptr;
            m_EndCap    = nullptr;
            return result;
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
