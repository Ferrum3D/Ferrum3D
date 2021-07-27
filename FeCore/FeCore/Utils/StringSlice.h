#pragma once
#include <cassert>
#include <utf8/utf8.h>
#include <utf8h/utf8.h>

namespace FE
{
    using TChar      = char;
    using TCodepoint = uint32_t;

    class StringSlice final
    {
        const TChar* m_Data;
        size_t m_Size;

    public:
        class Iterator
        {
            const TChar* m_Iter;
            const TChar* m_Begin;
            const TChar* m_End;

        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = TCodepoint;
            using pointer           = const TCodepoint*;
            using reference         = const TCodepoint&;

            inline Iterator(const TChar* begin, const TChar* end)
                : m_Iter(begin)
                , m_Begin(begin)
                , m_End(end)
            {
            }

            inline Iterator(const TChar* iter, const TChar* begin, const TChar* end)
                : m_Iter(iter)
                , m_Begin(begin)
                , m_End(end)
            {
            }

            inline value_type operator*() const
            {
                return utf8::peek_next(m_Iter, m_End);
            }

            inline Iterator& operator++()
            {
                utf8::next(m_Iter, m_End);
                return *this;
            }

            inline Iterator operator++(int)
            {
                Iterator t = *this;
                ++(*this);
                return t;
            }

            inline Iterator& operator--()
            {
                utf8::prior(m_Iter, m_Begin);
                return *this;
            }

            inline Iterator operator--(int)
            {
                Iterator t = *this;
                utf8::prior(m_Iter, m_Begin);
                return t;
            }

            inline friend bool operator==(const Iterator& a, const Iterator& b)
            {
                return a.m_Iter == b.m_Iter;
            };

            inline friend bool operator!=(const Iterator& a, const Iterator& b)
            {
                return a.m_Iter != b.m_Iter;
            };
        };

        inline constexpr StringSlice(const TChar* data, size_t size) noexcept
            : m_Data(data)
            , m_Size(size)
        {
        }

        inline constexpr StringSlice(const TChar* data) noexcept
            : m_Data(data)
            , m_Size(std::char_traits<TChar>::length(data))
        {
        }

        inline constexpr const TChar* Data() const noexcept
        {
            return m_Data;
        }

        inline constexpr size_t Size() const noexcept
        {
            return m_Size;
        }

        // O(N)
        inline size_t Length() const noexcept
        {
            auto ptr = Data();
            return utf8::distance(ptr, ptr + Size());
        }

        // O(1)
        inline TChar ByteAt(size_t index) const
        {
            assert(index < Size());
            return Data()[index];
        }

        // O(N)
        inline TCodepoint CodePointAt(size_t index) const
        {
            auto begin     = Data();
            auto end       = begin + Size() + 1;
            size_t cpIndex = 0;
            for (auto iter = begin; iter != end; utf8::next(iter, end), ++cpIndex)
            {
                if (cpIndex == index)
                    return utf8::peek_next(iter, end);
            }

            assert(0);
            return 0;
        }

        inline int Compare(const StringSlice& other) const noexcept
        {
            return utf8ncmp(Data(), other.Data(), std::min(Size(), other.Size()));
        }

        inline Iterator begin() const noexcept
        {
            auto ptr = Data();
            return Iterator(ptr, ptr + Size());
        }

        inline Iterator end() const noexcept
        {
            auto ptr  = Data();
            auto size = Size();
            return Iterator(ptr + size, ptr, ptr + size);
        }

        inline Iterator cbegin() const noexcept
        {
            auto ptr = Data();
            return Iterator(ptr, ptr + Size());
        }

        inline Iterator cend() const noexcept
        {
            auto ptr  = Data();
            auto size = Size();
            return Iterator(ptr + size, ptr, ptr + size);
        }
    };

    inline bool operator==(const StringSlice& lhs, const StringSlice& rhs) noexcept
    {
        return lhs.Size() == rhs.Size() && lhs.Compare(rhs) == 0;
    }

    inline bool operator!=(const StringSlice& lhs, const StringSlice& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    inline bool operator<(const StringSlice& lhs, const StringSlice& rhs) noexcept
    {
        return lhs.Compare(rhs) < 0;
    }

    inline bool operator>(const StringSlice& lhs, const StringSlice& rhs) noexcept
    {
        return lhs.Compare(rhs) > 0;
    }

    inline bool operator<=(const StringSlice& lhs, const StringSlice& rhs) noexcept
    {
        return lhs.Size() == rhs.Size() && lhs.Compare(rhs) <= 0;
    }

    inline bool operator>=(const StringSlice& lhs, const StringSlice& rhs) noexcept
    {
        return lhs.Size() == rhs.Size() && lhs.Compare(rhs) >= 0;
    }

} // namespace FE
