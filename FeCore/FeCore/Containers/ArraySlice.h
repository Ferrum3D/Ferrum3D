#pragma once
#include <FeCore/Memory/Memory.h>
#include <array>

namespace FE
{
    template<class T>
    class ArraySlice
    {
        const T* m_Begin;
        const T* m_End;

    public:
        FE_STRUCT_RTTI(ArraySlice, "C047D694-0887-403D-AD65-6A1B7B873951");

        inline ArraySlice()
            : m_Begin(nullptr)
            , m_End(nullptr)
        {
        }

        inline ArraySlice(const T* array, size_t length)
            : m_Begin(array)
            , m_End(array + length)
        {
            if (Empty())
            {
                Reset();
            }
        }

        inline ArraySlice(const T* begin, const T* end)
            : m_Begin(begin)
            , m_End(end)
        {
        }

        inline ArraySlice(const ArraySlice& other)
            : ArraySlice(other.m_Begin, other.m_End)
        {
        }

        inline ArraySlice(ArraySlice&& other) noexcept
            : ArraySlice(other.m_Begin, other.m_End)
        {
            other.Reset();
        }

        template<size_t N>
        inline ArraySlice(const std::array<T, N>& array) // NOLINT
            : m_Begin(array.data())
            , m_End(array.data() + array.size())
        {
        }

        inline ArraySlice(const Vector<T>& vector) // NOLINT
            : m_Begin(vector.data())
            , m_End(vector.data() + vector.size())
        {
        }

        inline ArraySlice& operator=(const ArraySlice& other) = default;

        inline ArraySlice& operator=(ArraySlice&& other) noexcept
        {
            *this = other;
            other.Reset();
        }

        inline ArraySlice operator()(size_t beginIndex, size_t endIndex) const noexcept
        {
            FE_CORE_ASSERT(beginIndex < Length() && endIndex <= Length(), "Index out of range");
            return ArraySlice(m_Begin + beginIndex, m_Begin + endIndex);
        }

        inline const T& operator[](size_t index) const noexcept
        {
            FE_CORE_ASSERT(index < Length(), "Index out of range");
            return m_Begin[index];
        }

        [[nodiscard]] inline size_t Length() const
        {
            return m_End - m_Begin;
        }

        [[nodiscard]] inline bool Empty() const
        {
            return Length() == 0;
        }

        inline void Reset()
        {
            m_Begin = m_End = nullptr;
        }

        [[nodiscard]] inline const T* Data() const
        {
            return m_Begin;
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
