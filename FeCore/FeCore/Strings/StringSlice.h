#pragma once
#include <FeCore/Strings/FeUnicode.h>
#include <cassert>
#include <ostream>

namespace FE
{
    using UTF8::TChar;
    using UTF8::TCharTraits;
    using UTF8::TCodepoint;
    using UTF8::TCodepointTraits;

    class StringSlice final
    {
        const TChar* m_Data;
        size_t m_Size;

    public:
        class Iterator
        {
            friend class StringSlice;
            const TChar* m_Iter;

        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = TCodepoint;
            using pointer           = const TCodepoint*;
            using reference         = const TCodepoint&;

            inline Iterator(const TChar* begin, const TChar* end)
                : m_Iter(begin)
            {
            }

            inline Iterator(const TChar* iter, const TChar* begin, const TChar* end)
                : m_Iter(iter)
            {
            }

            inline value_type operator*() const
            {
                return UTF8::PeekDecode(m_Iter);
            }

            inline Iterator& operator++()
            {
                UTF8::Decode(m_Iter);
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
                UTF8::DecodePrior(m_Iter);
                return *this;
            }

            inline Iterator operator--(int)
            {
                Iterator t = *this;
                UTF8::DecodePrior(m_Iter);
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

        inline StringSlice(Iterator begin, Iterator end) noexcept
            : m_Data(begin.m_Iter)
            , m_Size(end.m_Iter - begin.m_Iter)
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
            return UTF8::Length(ptr, Size());
        }

        inline StringSlice operator()(size_t beginIndex, size_t endIndex) const
        {
            auto begin = Data();
            auto end   = Data();
            UTF8::Advance(begin, beginIndex);
            UTF8::Advance(end, endIndex);
            return StringSlice(begin, end - begin);
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
            for (auto iter = begin; iter != end; UTF8::Decode(iter), ++cpIndex)
            {
                if (cpIndex == index)
                    return UTF8::PeekDecode(iter);
            }

            assert(0);
            return 0;
        }

        inline Iterator FindFirstOf(TCodepoint search) const noexcept
        {
            auto e = end();
            for (auto iter = begin(); iter != e; ++iter)
            {
                if (*iter == search)
                    return iter;
            }
            return e;
        }

        inline int Compare(const StringSlice& other) const noexcept
        {
            return UTF8::Compare(Data(), other.Data(), std::min(Size(), other.Size()));
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

namespace std
{
    inline ostream& operator<<(ostream& stream, FE::StringSlice str)
    {
        return stream << std::string_view(str.Data(), str.Size());
    }
} // namespace std
