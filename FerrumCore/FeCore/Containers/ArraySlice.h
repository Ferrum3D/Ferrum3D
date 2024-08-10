#pragma once
#include <FeCore/Containers/ArraySliceMut.h>

namespace FE
{
    //! \brief An immutable non-owning slice of elements stored contiguously, e.g. in an array or vector.
    template<class T>
    class ArraySlice
    {
        const T* m_Begin;
        const T* m_End;

    public:
        FE_RTTI_Base(ArraySlice, "C047D694-0887-403D-AD65-6A1B7B873951");

        inline ArraySlice()
            : m_Begin(nullptr)
            , m_End(nullptr)
        {
        }

        //! \brief Create from a mutable slice.
        //! \see ArraySliceMut.
        inline explicit ArraySlice(const ArraySliceMut<T>& mutableSlice)
            : m_Begin(mutableSlice.Data())
            , m_End(mutableSlice.Data() + mutableSlice.Length())
        {
        }

        //! \brief Create from a C-style array.
        //!
        //! \param [in] array  - A pointer to the array.
        //! \param [in] length - Length of the array.
        inline ArraySlice(const T* array, uint32_t length)
            : m_Begin(array)
            , m_End(array + length)
        {
            if (Empty())
            {
                Reset();
            }
        }

        //! \brief Create from a C-style array.
        //!
        //! \param [in] begin - A pointer to the array.
        //! \param [in] end   - A pointer to the element after array that won't be included.
        inline ArraySlice(const T* begin, const T* end)
            : m_Begin(begin)
            , m_End(end)
        {
        }

        //! \brief Create from a std::initializer_list.
        //!
        //! \param [in] lst - An initializer list to create the ArraySlice from.
        inline ArraySlice(std::initializer_list<T> lst)
            : m_Begin(lst.begin())
            , m_End(lst.end())
        {
        }

        //! \brief Copy constructor.
        inline ArraySlice(const ArraySlice& other) = default;
        inline ArraySlice& operator=(const ArraySlice& other) = default;

        //! \brief Move constructor.
        inline ArraySlice(ArraySlice&& other) noexcept
            : ArraySlice(other.m_Begin, other.m_End)
        {
            other.Reset();
        }

        inline ArraySlice& operator=(ArraySlice&& other) noexcept
        {
            *this = other;
            other.Reset();
            return *this;
        }

        //! \brief Create from a std::array.
        //!
        //! \tparam N - Length of the array.
        //!
        //! \param [in] array - The array.
        template<size_t N>
        inline ArraySlice(const std::array<T, N>& array) // NOLINT
            : m_Begin(array.data())
            , m_End(array.data() + array.size())
        {
        }

        //! \brief Create from a Vector<T>.
        //!
        //! \param [in] vector - The vector.
        inline ArraySlice(const eastl::vector<T>& vector) // NOLINT
            : m_Begin(vector.data())
            , m_End(vector.data() + vector.size())
        {
        }

        //! \brief Create a subslice.
        //!
        //! \param [in] beginIndex - First index of the subslice to create.
        //! \param [in] endIndex   - The first index past the subslice to create.
        //!
        //! \return The created subslice.
        inline ArraySlice operator()(uint32_t beginIndex, uint32_t endIndex) const noexcept
        {
            FE_CORE_ASSERT(beginIndex < Length() && endIndex <= Length(), "Index out of range");
            return ArraySlice(m_Begin + beginIndex, m_Begin + endIndex);
        }

        inline const T& operator[](uint32_t index) const noexcept
        {
            FE_CORE_ASSERT(index < Length(), "Index out of range");
            return m_Begin[index];
        }

        [[nodiscard]] inline ptrdiff_t IndexOf(const T& value) const
        {
            auto length = static_cast<ptrdiff_t>(Length());
            for (ptrdiff_t i = 0; i < length; ++i)
            {
                if (value == m_Begin[i])
                {
                    return i;
                }
            }

            return -1;
        }

        [[nodiscard]] inline bool Contains(const T& value)
        {
            return IndexOf(value) != -1;
        }

        //! \bried Length of the slice.
        [[nodiscard]] inline uint32_t Length() const
        {
            return static_cast<uint32_t>(m_End - m_Begin);
        }

        //! \brief Check if the slice is empty.
        [[nodiscard]] inline bool Empty() const
        {
            return Length() == 0;
        }

        //! \brief Reset the slice to empty state.
        inline void Reset()
        {
            m_Begin = m_End = nullptr;
        }

        //! \brief Get pointer to the beginning of the slice.
        [[nodiscard]] inline const T* Data() const
        {
            return m_Begin;
        }

        //! \brief Copy data to a mutable slice.
        //!
        //! Copies min(this.Length(), destination.Length()) elements.
        //!
        //! \param [in] destination - The slice to copy the data to.
        //!
        //! \return The number of elements copied.
        inline uint32_t CopyDataTo(ArraySliceMut<T> destination) const
        {
            const uint32_t size = std::min(Length(), destination.Length());
            memcpy(destination.Data(), Data(), size * sizeof(T));
            return size;
        }

        [[nodiscard]] inline const T* begin() const
        {
            return m_Begin;
        }

        [[nodiscard]] inline const T* end() const
        {
            return m_End;
        }

        inline bool operator==(const ArraySlice& other) const noexcept
        {
            return m_Begin == other.m_Begin && m_End == other.m_End;
        }

        inline bool operator!=(const ArraySlice& other) const noexcept
        {
            return !(*this == other);
        }
    };
} // namespace FE
