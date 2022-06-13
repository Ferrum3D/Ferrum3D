#pragma once
#include <FeCore/Memory/Memory.h>
#include <array>

namespace FE
{
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

        inline ArraySliceMut(T* array, USize length)
            : m_Begin(array)
            , m_End(array + length)
        {
            if (Empty())
            {
                Reset();
            }
        }

        inline ArraySliceMut(T* begin, T* end)
            : m_Begin(begin)
            , m_End(end)
        {
        }

        inline ArraySliceMut(const ArraySliceMut& other) = default;
        inline ArraySliceMut& operator=(const ArraySliceMut& other) = default;

        inline ArraySliceMut(ArraySliceMut&& other) noexcept
            : ArraySliceMut(other.m_Begin, other.m_End)
        {
            other.Reset();
        }

        template<USize N>
        inline explicit ArraySliceMut(std::array<T, N>& array)
            : m_Begin(array.data())
            , m_End(array.data() + array.size())
        {
        }

        inline explicit ArraySliceMut(Vector<T>& vector)
            : m_Begin(vector.data())
            , m_End(vector.data() + vector.size())
        {
        }

        inline ArraySliceMut& operator=(ArraySliceMut&& other) noexcept
        {
            *this = other;
            other.Reset();
        }

        inline ArraySliceMut operator()(USize beginIndex, USize endIndex) noexcept
        {
            FE_CORE_ASSERT(beginIndex < Length() && endIndex <= Length(), "Index out of range");
            return ArraySliceMut(m_Begin + beginIndex, m_Begin + endIndex);
        }

        inline T& operator[](USize index) noexcept
        {
            FE_CORE_ASSERT(index < Length(), "Index out of range");
            return m_Begin[index];
        }

        [[nodiscard]] inline USize Length() const
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

        [[nodiscard]] inline T* Data()
        {
            return m_Begin;
        }

        inline USize CopyDataTo(ArraySliceMut<T> destination) const
        {
            USize size = std::min(Length(), destination.Length());
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
