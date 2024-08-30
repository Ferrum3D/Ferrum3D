#pragma once
#include <FeCore/Modules/Environment.h>
#include <FeCore/Strings/StringBase.h>
#include <FeCore/Utils/Result.h>
#include <cassert>
#include <codecvt>
#include <locale>
#include <ostream>
#include <string>
#include <type_traits>

namespace FE
{
    enum class ParseErrorCode : uint32_t
    {
        None,
        Overflow,
        InvalidSyntax,
        UnexpectedEnd
    };

    class ParseError final
    {
        uint32_t m_ErrorCode : 4;
        uint32_t m_Position : 28;

    public:
        inline ParseError(ParseErrorCode code) noexcept // NOLINT(google-explicit-constructor)
            : ParseError(code, 0)
        {
        }

        inline ParseError(ParseErrorCode code, uint32_t position) noexcept
            : m_ErrorCode(enum_cast(code))
            , m_Position(position)
        {
        }

        [[nodiscard]] inline ParseErrorCode GetCode() const noexcept
        {
            return static_cast<ParseErrorCode>(m_ErrorCode);
        }

        [[nodiscard]] inline uint32_t GetPosition() const noexcept
        {
            return m_Position;
        }

        inline friend bool operator==(ParseError parseError, ParseErrorCode code) noexcept
        {
            return parseError.GetCode() == code;
        }

        inline friend bool operator!=(ParseError parseError, ParseErrorCode code) noexcept
        {
            return parseError.GetCode() != code;
        }

        inline friend bool operator==(ParseError lhs, ParseError rhs) noexcept
        {
            return lhs.GetCode() == rhs.GetCode() && lhs.GetPosition() == rhs.GetPosition();
        }

        inline friend bool operator!=(ParseError lhs, ParseError rhs) noexcept
        {
            return lhs.GetCode() != rhs.GetCode() && lhs.GetPosition() != rhs.GetPosition();
        }
    };

    template<class T>
    struct ValueParser : std::false_type
    {
    };

    //! @brief A slice of a String.
    class StringSlice final
    {
        const TChar* m_Data;
        uint32_t m_Size;

        ParseError TryToIntImpl(int64_t& result) const;
        ParseError TryToUIntImpl(uint64_t& result) const;

        ParseError TryToFloatImpl(double& result) const;

        template<class T>
        inline static constexpr bool is_signed_integer_v =
            std::is_signed_v<T> && std::is_integral_v<T> && !std::is_same_v<T, bool>;
        template<class T>
        inline static constexpr bool is_unsigned_integer_v =
            std::is_unsigned_v<T> && std::is_integral_v<T> && !std::is_same_v<T, bool>;

        template<class TInt>
        [[nodiscard]] inline std::enable_if_t<is_signed_integer_v<TInt>, ParseError> ParseImpl(TInt& result) const noexcept
        {
            int64_t temp;
            auto ret = TryToIntImpl(temp);
            if (ret != ParseErrorCode::None)
            {
                return ret;
            }

            result = static_cast<TInt>(temp);
            if (result != temp)
            {
                return ParseErrorCode::Overflow;
            }

            return ParseErrorCode::None;
        }

        template<class TInt>
        [[nodiscard]] inline std::enable_if_t<is_unsigned_integer_v<TInt>, ParseError> ParseImpl(TInt& result) const noexcept
        {
            uint64_t temp;
            auto ret = TryToUIntImpl(temp);
            if (ret != ParseErrorCode::None)
            {
                return ret;
            }

            result = static_cast<TInt>(temp);
            if (result != temp)
            {
                return ParseErrorCode::Overflow;
            }

            return ParseErrorCode::None;
        }

        template<class TFloat>
        [[nodiscard]] inline std::enable_if_t<std::is_floating_point_v<TFloat>, ParseError> ParseImpl(
            TFloat& result) const noexcept
        {
            double temp;
            auto ret = TryToFloatImpl(temp);
            result = static_cast<TFloat>(temp);
            return ret;
        }

        template<class TBool>
        [[nodiscard]] inline std::enable_if_t<std::is_same_v<TBool, bool>, ParseError> ParseImpl(TBool& result) const noexcept
        {
            // something weird here:
            // error C2678: binary '==': no operator found which takes a left-hand operand of type
            //     'const FE::StringSlice' (or there is no acceptable conversion)
            if (std::string_view{ m_Data, m_Size } == std::string_view{ "true" }
                || std::string_view{ m_Data, m_Size } == std::string_view{ "1" })
            {
                result = true;
                return ParseErrorCode::None;
            }
            if (std::string_view{ m_Data, m_Size } == std::string_view{ "false" }
                || std::string_view{ m_Data, m_Size } == std::string_view{ "0" })
            {
                result = false;
                return ParseErrorCode::None;
            }

            return ParseErrorCode::InvalidSyntax;
        }

        template<class T>
        [[nodiscard]] inline std::enable_if_t<ValueParser<T>::value, ParseError> ParseImpl(T& result) const noexcept
        {
            return ValueParser<T>::TryConvert(*this, result);
        }

    public:
        using Iterator = Internal::StrIterator;

        inline constexpr StringSlice() noexcept
            : m_Data(nullptr)
            , m_Size(0)
        {
        }

        inline constexpr StringSlice(const TChar* data, uint32_t size) noexcept
            : m_Data(data)
            , m_Size(size)
        {
        }

        inline constexpr StringSlice(std::string_view stringView) noexcept
            : m_Data(stringView.data())
            , m_Size(static_cast<uint32_t>(stringView.size()))
        {
        }

        inline StringSlice(Env::Name name) noexcept
        {
            if (name.Valid())
            {
                const Env::Name::Record* pRecord = name.GetRecord();
                m_Data = pRecord->Data;
                m_Size = pRecord->Size;
            }
            else
            {
                m_Data = nullptr;
                m_Size = 0;
            }
        }

        inline constexpr StringSlice(const TChar* data) noexcept
            : m_Data(data)
            , m_Size(data == nullptr ? 0 : Str::ByteLength(data))
        {
        }

        template<size_t S>
        inline constexpr StringSlice(const TChar (&data)[S]) noexcept
            : m_Data(data)
            , m_Size(Str::ByteLength(data))
        {
        }

        inline StringSlice(Iterator begin, Iterator end) noexcept
            : m_Data(begin.m_Iter)
            , m_Size(static_cast<uint32_t>(end.m_Iter - begin.m_Iter))
        {
        }

        [[nodiscard]] inline constexpr const TChar* Data() const noexcept
        {
            return m_Data;
        }

        [[nodiscard]] inline constexpr uint32_t Size() const noexcept
        {
            return m_Size;
        }

        // O(N)
        [[nodiscard]] inline uint32_t Length() const noexcept
        {
            return Str::Length(Data(), Size());
        }

        inline StringSlice Substring(uint32_t beginIndex, uint32_t length) const
        {
            auto begin = Data();
            auto end = Data();
            UTF8::Advance(begin, beginIndex);
            UTF8::Advance(end, beginIndex + length);
            return StringSlice(begin, static_cast<uint32_t>(end - begin));
        }

        // O(1)
        [[nodiscard]] inline TChar ByteAt(uint32_t index) const
        {
            assert(index < Size());
            return Data()[index];
        }

        // O(N)
        [[nodiscard]] inline TCodepoint CodePointAt(uint32_t index) const
        {
            return Str::CodepointAt(Data(), Size(), index);
        }

        [[nodiscard]] inline Iterator FindFirstOf(Iterator start, TCodepoint search) const noexcept
        {
            const uint32_t size = Size();
            const TChar* data = Data();
            FE_CORE_ASSERT(start.m_Iter >= data && start.m_Iter <= data + size, "");

            const uint32_t searchSize = static_cast<uint32_t>(data + size - start.m_Iter);
            return Str::FindFirstOf(start.m_Iter, searchSize, search);
        }

        [[nodiscard]] inline Iterator FindFirstOf(TCodepoint search) const noexcept
        {
            return Str::FindFirstOf(Data(), Size(), search);
        }

        [[nodiscard]] inline Iterator FindLastOf(TCodepoint search) const noexcept
        {
            return Str::FindLastOf(Data(), Size(), search);
        }

        [[nodiscard]] inline bool Contains(TCodepoint search) const noexcept
        {
            return FindFirstOf(search) != end();
        }

        [[nodiscard]] inline bool StartsWith(StringSlice prefix, bool caseSensitive = true) const noexcept
        {
            if (prefix.Size() > Size())
                return false;

            return UTF8::AreEqual(Data(), prefix.Data(), prefix.Size(), prefix.Size(), caseSensitive);
        }

        [[nodiscard]] inline bool EndsWith(StringSlice suffix, bool caseSensitive = true) const noexcept
        {
            if (suffix.Size() > Size())
                return false;

            return UTF8::AreEqual(Data() + Size() - suffix.Size(), suffix.Data(), suffix.Size(), suffix.Size(), caseSensitive);
        }

        [[nodiscard]] inline festd::pmr::vector<StringSlice> Split(TCodepoint c = ' ',
                                                                   std::pmr::memory_resource* pAllocator = nullptr) const
        {
            if (pAllocator == nullptr)
                pAllocator = std::pmr::get_default_resource();

            festd::pmr::vector<StringSlice> result{ pAllocator };
            auto current = begin();
            while (current != end())
            {
                auto cPos = FindFirstOf(current, c);
                result.emplace_back(current.m_Iter, cPos.m_Iter - current.m_Iter);
                current = cPos;
                if (current != end())
                    ++current;
            }

            return result;
        }

        [[nodiscard]] inline festd::pmr::vector<StringSlice> SplitLines(std::pmr::memory_resource* pAllocator = nullptr) const
        {
            if (pAllocator == nullptr)
                pAllocator = std::pmr::get_default_resource();

            festd::pmr::vector<StringSlice> result;
            auto current = begin();
            while (current != end())
            {
                auto cPos = FindFirstOf(current, '\n');
                auto line = StringSlice(current.m_Iter, static_cast<uint32_t>(cPos.m_Iter - current.m_Iter)).StripRight("\r");
                result.push_back(line);
                current = cPos;
                if (current != end())
                    ++current;
            }

            return result;
        }

        [[nodiscard]] inline StringSlice StripLeft(StringSlice chars = "\n\r\t ") const noexcept
        {
            if (Size() == 0)
                return {};

            auto endIter = end();
            auto result = begin();
            for (auto iter = begin(); iter != endIter; ++iter)
            {
                if (!chars.Contains(*iter))
                    break;

                result = iter;
                ++result;
            }

            return { result, endIter };
        }

        [[nodiscard]] inline StringSlice StripRight(StringSlice chars = "\n\r\t ") const noexcept
        {
            if (Size() == 0)
                return {};

            auto beginIter = begin();
            auto result = end();
            for (auto iter = --end(); iter != beginIter; --iter)
            {
                if (!chars.Contains(*iter))
                    break;

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

        [[nodiscard]] inline bool IsEqualTo(const StringSlice& other, bool caseSensitive = true) const noexcept
        {
            return UTF8::AreEqual(Data(), other.Data(), Size(), other.Size(), caseSensitive);
        }

        template<class T>
        [[nodiscard]] inline Result<T, ParseError> Parse() const
        {
            T result;
            auto err = ParseImpl(result);
            if (err == ParseErrorCode::None)
            {
                return result;
            }

            return Err(err);
        }

        [[nodiscard]] inline explicit operator UUID() const noexcept
        {
            return UUID(Data());
        }

        [[nodiscard]] inline explicit operator Env::Name() const noexcept
        {
            return Env::Name{ Data(), static_cast<uint32_t>(Size()) };
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


    [[nodiscard]] inline uint64_t DefaultHash(StringSlice str) noexcept
    {
        return DefaultHash(str.Data(), str.Size());
    }


    template<>
    struct ValueParser<UUID> : std::true_type
    {
        inline static ParseError TryConvert(const StringSlice& str, UUID& result)
        {
            if (str.Length() != 36)
            {
                return { ParseErrorCode::UnexpectedEnd, str.Length() - 1 };
            }

            return UUID::TryParse(str.Data(), result, false) ? ParseErrorCode::None : ParseErrorCode::InvalidSyntax;
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
