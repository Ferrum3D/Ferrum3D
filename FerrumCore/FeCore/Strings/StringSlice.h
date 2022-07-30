#pragma once
#include <FeCore/Containers/List.h>
#include <FeCore/Strings/FeUnicode.h>
#include <cassert>
#include <codecvt>
#include <locale>
#include <ostream>
#include <string>
#include <type_traits>

namespace FE
{
    using UTF8::TChar;
    using UTF8::TCharTraits;
    using UTF8::TCodepoint;
    using UTF8::TCodepointTraits;

    template<class T>
    struct ValueParser : std::false_type
    {
    };

    //! \brief A slice of \ref String.
    class StringSlice final
    {
        const TChar* m_Data;
        size_t m_Size;

        bool TryToIntImpl(Int64& result) const;
        bool TryToUIntImpl(UInt64& result) const;

        bool TryToFloatImpl(Float64& result) const;

        template<class T>
        inline static constexpr bool is_signed_integer_v =
            std::is_signed_v<T>&& std::is_integral_v<T> && !std::is_same_v<T, bool>;
        template<class T>
        inline static constexpr bool is_unsigned_integer_v =
            std::is_unsigned_v<T>&& std::is_integral_v<T> && !std::is_same_v<T, bool>;

    public:
        FE_STRUCT_RTTI(StringSlice, "DCBAE48D-8751-4F0C-96F9-99866394482B");

        class Iterator
        {
            friend class StringSlice;
            friend class String;
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

        inline constexpr StringSlice(const TChar* data, USize size) noexcept
            : m_Data(data)
            , m_Size(size)
        {
        }

        inline constexpr StringSlice(const std::string_view& stringView) noexcept
            : m_Data(stringView.data())
            , m_Size(stringView.size())
        {
        }

        inline constexpr StringSlice(const TChar* data) noexcept
            : m_Data(data)
            , m_Size(data == nullptr ? 0 : std::char_traits<TChar>::length(data))
        {
        }

        template<USize S>
        inline constexpr StringSlice(const TChar (&data)[S]) noexcept
            : m_Data(data)
            , m_Size(S)
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

        [[nodiscard]] inline Iterator FindFirstOf(Iterator start, TCodepoint search) const noexcept
        {
            auto e = end();
            for (auto iter = start; iter != e; ++iter)
            {
                if (*iter == search)
                {
                    return iter;
                }
            }
            return e;
        }

        [[nodiscard]] inline Iterator FindFirstOf(TCodepoint search) const noexcept
        {
            return FindFirstOf(begin(), search);
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

        [[nodiscard]] inline bool Contains(TCodepoint search) const noexcept
        {
            return FindFirstOf(search) != end();
        }

        [[nodiscard]] inline bool StartsWith(StringSlice prefix) const noexcept
        {
            if (prefix.Size() > Size())
            {
                return false;
            }

            return UTF8::Compare(Data(), prefix.Data(), prefix.Size(), prefix.Size()) == 0;
        }

        [[nodiscard]] inline bool EndsWith(StringSlice suffix) const noexcept
        {
            if (suffix.Size() > Size())
            {
                return false;
            }

            return UTF8::Compare(Data() + Size() - suffix.Size(), suffix.Data(), suffix.Size(), suffix.Size()) == 0;
        }

        [[nodiscard]] inline List<StringSlice> Split(TCodepoint c = ' ') const
        {
            List<StringSlice> result;
            auto current = begin();
            while (current != end())
            {
                auto cPos = FindFirstOf(current, c);
                result.Emplace(current.m_Iter, cPos.m_Iter - current.m_Iter);
                current = cPos;
                if (current != end())
                {
                    ++current;
                }
            }

            return result;
        }

        [[nodiscard]] inline List<StringSlice> SplitLines() const
        {
            List<StringSlice> result;
            auto current = begin();
            while (current != end())
            {
                auto cPos = FindFirstOf(current, '\n');
                auto line = StringSlice(current.m_Iter, cPos.m_Iter - current.m_Iter).StripRight("\r");
                result.Emplace(line);
                current = cPos;
                if (current != end())
                {
                    ++current;
                }
            }

            return result;
        }

        [[nodiscard]] inline StringSlice StripLeft(StringSlice chars = "\n\r\t ") const noexcept
        {
            if (Size() == 0)
            {
                return {};
            }

            auto endIter = end();
            auto result  = begin();
            for (auto iter = begin(); iter != endIter; ++iter)
            {
                if (!chars.Contains(*iter))
                {
                    break;
                }

                result = iter;
                ++result;
            }

            return { result, endIter };
        }

        [[nodiscard]] inline StringSlice StripRight(StringSlice chars = "\n\r\t ") const noexcept
        {
            if (Size() == 0)
            {
                return {};
            }

            auto beginIter = begin();
            auto result    = end();
            for (auto iter = --end(); iter != beginIter; --iter)
            {
                if (!chars.Contains(*iter))
                {
                    break;
                }

                result = iter;
            }

            return { beginIter, result };
        }

        [[nodiscard]] inline StringSlice Strip(StringSlice chars = "\n\r\t ") const noexcept
        {
            return StripLeft(chars).StripRight(chars);
        }

        [[nodiscard]] inline int Compare(const StringSlice& other) const noexcept
        {
            return UTF8::Compare(Data(), other.Data(), Size(), other.Size());
        }

        template<class TInt>
        [[nodiscard]] inline std::enable_if_t<is_signed_integer_v<TInt>, TInt> TryConvertTo(TInt& result) const noexcept
        {
            Int64 temp;
            auto ret = TryToIntImpl(temp);
            result   = static_cast<TInt>(temp);
            return ret && result == temp;
        }

        template<class TInt>
        [[nodiscard]] inline std::enable_if_t<is_unsigned_integer_v<TInt>, TInt> TryConvertTo(TInt& result) const noexcept
        {
            UInt64 temp;
            auto ret = TryToUIntImpl(temp);
            result   = static_cast<TInt>(temp);
            return ret && result == temp;
        }

        template<class TFloat>
        [[nodiscard]] inline std::enable_if_t<std::is_floating_point_v<TFloat>, TFloat> TryConvertTo(
            TFloat& result) const noexcept
        {
            Float64 temp;
            auto ret = TryToFloatImpl(temp);
            result   = static_cast<TFloat>(temp);
            return ret;
        }

        template<class TBool>
        [[nodiscard]] inline std::enable_if_t<std::is_same_v<TBool, bool>, TBool> TryConvertTo(TBool& result) const noexcept
        {
            if (*this == "true" || *this == "1")
            {
                result = true;
                return true;
            }
            if (*this == "false" || *this == "0")
            {
                result = false;
                return true;
            }

            return false;
        }

        template<class T>
        [[nodiscard]] inline std::enable_if_t<ValueParser<T>::value, bool> TryConvertTo(T& result) const noexcept
        {
            return ValueParser<T>::TryConvert(*this, result);
        }

        template<class T>
        [[nodiscard]] inline T ConvertTo() const
        {
            T result;
            FE_CORE_ASSERT(TryConvertTo(result), "Attempt to parse an invalid value");
            return result;
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

        [[nodiscard]] inline explicit operator UUID() const noexcept
        {
            return UUID(Data());
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

    template<>
    struct ValueParser<UUID> : std::true_type
    {
        inline static bool TryConvert(const StringSlice& str, UUID& result)
        {
            if (str.Length() != 36)
            {
                return false;
            }

            return UUID::TryParse(str.Data(), result, false);
        }
    };
} // namespace FE

namespace std
{
    inline ostream& operator<<(ostream& stream, FE::StringSlice str)
    {
        return stream << std::string_view(str.Data(), str.Size());
    }
} // namespace std
