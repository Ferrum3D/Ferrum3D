#pragma once
#include <FeCore/Strings/FeUnicode.h>
#include <cassert>
#include <codecvt>
#include <locale>
#include <ostream>
#include <string>

namespace FE
{
    using UTF8::TChar;
    using UTF8::TCharTraits;
    using UTF8::TCodepoint;
    using UTF8::TCodepointTraits;

    //! \brief A slice of \ref String.
    class StringSlice final
    {
        const TChar* m_Data;
        size_t m_Size;

    public:
        FE_STRUCT_RTTI(StringSlice, "DCBAE48D-8751-4F0C-96F9-99866394482B");

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

            inline Iterator(const TChar* iter)
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

            inline friend Iterator operator+(Iterator lhs, Int32 rhs)
            {
                if (rhs > 0)
                {
                    while (rhs--)
                        ++lhs;
                }
                else
                {
                    while (rhs--)
                        --lhs;
                }

                return lhs;
            }

            inline friend Iterator operator-(const Iterator& lhs, Int32 rhs)
            {
                return lhs + (-rhs);
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

        inline constexpr StringSlice() noexcept
            : m_Data(nullptr)
            , m_Size(0)
        {
        }

        inline constexpr StringSlice(const TChar* data, size_t size) noexcept
            : m_Data(data)
            , m_Size(size)
        {
        }

        inline constexpr StringSlice(const TChar* data) noexcept
            : m_Data(data)
            , m_Size(data == nullptr ? 0 : std::char_traits<TChar>::length(data))
        {
        }

        inline StringSlice(Iterator begin, Iterator end) noexcept
            : m_Data(begin.m_Iter)
            , m_Size(end.m_Iter - begin.m_Iter)
        {
        }

        [[nodiscard]] inline constexpr const TChar* Data() const noexcept
        {
            return m_Data;
        }

        [[nodiscard]] inline constexpr size_t Size() const noexcept
        {
            return m_Size;
        }

        // O(N)
        [[nodiscard]] inline size_t Length() const noexcept
        {
            return UTF8::Length(Data(), Size());
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
        [[nodiscard]] inline TChar ByteAt(size_t index) const
        {
            assert(index < Size());
            return Data()[index];
        }

        // O(N)
        [[nodiscard]] inline TCodepoint CodePointAt(size_t index) const
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

        [[nodiscard]] inline Iterator FindFirstOf(TCodepoint search) const noexcept
        {
            auto e = end();
            for (auto iter = begin(); iter != e; ++iter)
            {
                if (*iter == search)
                {
                    return iter;
                }
            }
            return e;
        }

        [[nodiscard]] inline Iterator FindLastOf(TCodepoint search) const noexcept
        {
            auto e      = end();
            auto result = e;
            for (auto iter = begin(); iter != e; ++iter)
            {
                if (*iter == search)
                {
                    result = iter;
                }
            }
            return result;
        }

        [[nodiscard]] inline int Compare(const StringSlice& other) const noexcept
        {
            return UTF8::Compare(Data(), other.Data(), Size(), other.Size());
        }

        [[nodiscard]] inline std::wstring ToWideString() const
        {
            std::wstring result;
            result.reserve(Length());
            for (TCodepoint cp : *this)
            {
                result += static_cast<wchar_t>(cp);
            }

            return result;
        }

        [[nodiscard]] inline Iterator begin() const noexcept
        {
            return Iterator(Data());
        }

        [[nodiscard]] inline Iterator end() const noexcept
        {
            return Iterator(Data() + Size());
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