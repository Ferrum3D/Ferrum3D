#pragma once
#include <EASTL/array.h>
#include <FeCore/Memory/Memory.h>

namespace FE
{
    //! \brief A mutable non-owning slice of elements stored contiguously, e.g. in an array or List<T>.
    template<class T>
    class ArraySliceMut
    {
        T* m_Begin;
        T* m_End;

    public:
        FE_STRUCT_RTTI(ArraySliceMut, "086387CB-9142-4A0D-9037-2B6044BE5875");

        inline ArraySliceMut()
            : m_Begin(nullptr)
            , m_End(nullptr)
        {
        }

        //! \brief Create from a C-style array.
        //!
        //! \param [in] array - A pointer to the array.
        //! \param [in] length - Length of the array.
        inline ArraySliceMut(T* array, uint32_t length)
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
        //! \param [in] end - A pointer to the element after array that won't be included.
        inline ArraySliceMut(T* begin, T* end)
            : m_Begin(begin)
            , m_End(end)
        {
        }

        //! \brief Copy constructor.
        inline ArraySliceMut(const ArraySliceMut& other) = default;
        inline ArraySliceMut& operator=(const ArraySliceMut& other) = default;

        //! \brief Move constructor.
        inline ArraySliceMut(ArraySliceMut&& other) noexcept = default;
        inline ArraySliceMut& operator=(ArraySliceMut&& other) noexcept = default;

        //! \brief Create from a std::array.
        //!
        //! \tparam N - Length of the array.
        //! \param [in] array - The array.
        template<USize N>
        inline explicit ArraySliceMut(eastl::array<T, N>& array)
            : m_Begin(array.data())
            , m_End(array.data() + array.size())
        {
        }

        //! \brief Create from a Vector<T>.
        //!
        //! \param [in] vector - The vector.
        inline explicit ArraySliceMut(eastl::vector<T>& vector)
            : m_Begin(vector.data())
            , m_End(vector.data() + vector.size())
        {
        }

        //! \brief Create a subslice.
        //!
        //! \param [in] beginIndex - First index of the subslice to create.
        //! \param [in] endIndex - The first index past the subslice to create.
        //!
        //! \return The created subslice.
        inline ArraySliceMut operator()(uint32_t beginIndex, uint32_t endIndex) noexcept
        {
            FE_CORE_ASSERT(beginIndex < Length() && endIndex <= Length(), "Index out of range");
            return ArraySliceMut(m_Begin + beginIndex, m_Begin + endIndex);
        }

        inline T& operator[](uint32_t index) noexcept
        {
            FE_CORE_ASSERT(index < Length(), "Index out of range");
            return m_Begin[index];
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

        //! \brief Get pointer to the beginning of the slice.
        [[nodiscard]] inline T* Data()
        {
            return m_Begin;
        }

        //! \brief Copy data to another slice.
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

        [[nodiscard]] inline T* begin()
        {
            return m_Begin;
        }

        [[nodiscard]] inline T* end()
        {
            return m_End;
        }

        inline bool operator==(const ArraySliceMut& other) const noexcept
        {
            return m_Begin == other.m_Begin && m_End == other.m_End;
        }

        inline bool operator!=(const ArraySliceMut& other) const noexcept
        {
            return !(*this == other);
        }
    };
} // namespace FE
