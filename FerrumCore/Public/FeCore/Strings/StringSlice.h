﻿#pragma once
#include <FeCore/Modules/Environment.h>
#include <FeCore/Strings/StringBase.h>
#include <FeCore/Utils/UUID.h>
#include <festd/vector.h>

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
        ParseError(ParseErrorCode code) noexcept // NOLINT(google-explicit-constructor)
            : ParseError(code, 0)
        {
        }

        ParseError(ParseErrorCode code, uint32_t position) noexcept
            : m_ErrorCode(festd::to_underlying(code))
            , m_Position(position)
        {
        }

        [[nodiscard]] ParseErrorCode GetCode() const noexcept
        {
            return static_cast<ParseErrorCode>(m_ErrorCode);
        }

        [[nodiscard]] uint32_t GetPosition() const noexcept
        {
            return m_Position;
        }

        friend bool operator==(ParseError parseError, ParseErrorCode code) noexcept
        {
            return parseError.GetCode() == code;
        }

        friend bool operator!=(ParseError parseError, ParseErrorCode code) noexcept
        {
            return parseError.GetCode() != code;
        }

        friend bool operator==(ParseError lhs, ParseError rhs) noexcept
        {
            return lhs.GetCode() == rhs.GetCode() && lhs.GetPosition() == rhs.GetPosition();
        }

        friend bool operator!=(ParseError lhs, ParseError rhs) noexcept
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
        const TChar* m_data;
        uint32_t m_size;

        ParseError TryToIntImpl(int64_t& result) const;
        ParseError TryToUIntImpl(uint64_t& result) const;

        ParseError TryToFloatImpl(double& result) const;

        template<class T>
        static constexpr bool is_signed_integer_v = std::is_signed_v<T> && std::is_integral_v<T> && !std::is_same_v<T, bool>;
        template<class T>
        static constexpr bool is_unsigned_integer_v = std::is_unsigned_v<T> && std::is_integral_v<T> && !std::is_same_v<T, bool>;

        template<class TInt>
        [[nodiscard]] std::enable_if_t<is_signed_integer_v<TInt>, ParseError> ParseImpl(TInt& result) const noexcept
        {
            int64_t temp;
            if (const ParseError ret = TryToIntImpl(temp); ret != ParseErrorCode::None)
                return ret;

            result = static_cast<TInt>(temp);
            if (result != temp)
                return ParseErrorCode::Overflow;

            return ParseErrorCode::None;
        }

        template<class TInt>
        [[nodiscard]] std::enable_if_t<is_unsigned_integer_v<TInt>, ParseError> ParseImpl(TInt& result) const noexcept
        {
            uint64_t temp;
            if (const ParseError ret = TryToUIntImpl(temp); ret != ParseErrorCode::None)
                return ret;

            result = static_cast<TInt>(temp);
            if (result != temp)
                return ParseErrorCode::Overflow;

            return ParseErrorCode::None;
        }

        template<class TFloat>
        [[nodiscard]] std::enable_if_t<std::is_floating_point_v<TFloat>, ParseError> ParseImpl(TFloat& result) const noexcept
        {
            double temp;
            const ParseError ret = TryToFloatImpl(temp);
            result = static_cast<TFloat>(temp);
            return ret;
        }

        template<class TBool>
        [[nodiscard]] std::enable_if_t<std::is_same_v<TBool, bool>, ParseError> ParseImpl(TBool& result) const noexcept
        {
            // something weird here:
            // error C2678: binary '==': no operator found which takes a left-hand operand of type
            //     'const FE::StringSlice' (or there is no acceptable conversion)
            if (std::string_view{ m_data, m_size } == std::string_view{ "true" }
                || std::string_view{ m_data, m_size } == std::string_view{ "1" })
            {
                result = true;
                return ParseErrorCode::None;
            }
            if (std::string_view{ m_data, m_size } == std::string_view{ "false" }
                || std::string_view{ m_data, m_size } == std::string_view{ "0" })
            {
                result = false;
                return ParseErrorCode::None;
            }

            return ParseErrorCode::InvalidSyntax;
        }

        template<class T>
        [[nodiscard]] std::enable_if_t<ValueParser<T>::value, ParseError> ParseImpl(T& result) const noexcept
        {
            return ValueParser<T>::TryConvert(*this, result);
        }

    public:
        using Iterator = Internal::StrIterator;

        constexpr StringSlice() noexcept
            : m_data(nullptr)
            , m_size(0)
        {
        }

        constexpr StringSlice(const TChar* data, uint32_t size) noexcept
            : m_data(data)
            , m_size(size)
        {
        }

        constexpr StringSlice(std::string_view stringView) noexcept
            : m_data(stringView.data())
            , m_size(static_cast<uint32_t>(stringView.size()))
        {
        }

        StringSlice(Env::Name name) noexcept
        {
            if (name.Valid())
            {
                const Env::Name::Record* pRecord = name.GetRecord();
                m_data = pRecord->m_data;
                m_size = pRecord->m_size;
            }
            else
            {
                m_data = nullptr;
                m_size = 0;
            }
        }

        constexpr StringSlice(const TChar* data) noexcept
            : m_data(data)
            , m_size(data == nullptr ? 0 : Str::ByteLength(data))
        {
        }

        template<size_t S>
        constexpr StringSlice(const TChar (&data)[S]) noexcept
            : m_data(data)
            , m_size(Str::ByteLength(data))
        {
        }

        StringSlice(Iterator begin, Iterator end) noexcept
            : m_data(begin.m_Iter)
            , m_size(static_cast<uint32_t>(end.m_Iter - begin.m_Iter))
        {
        }

        [[nodiscard]] constexpr const TChar* Data() const noexcept
        {
            return m_data;
        }

        [[nodiscard]] constexpr uint32_t Size() const noexcept
        {
            return m_size;
        }

        // O(N)
        [[nodiscard]] uint32_t Length() const noexcept
        {
            return Str::Length(Data(), Size());
        }

        StringSlice Substring(uint32_t beginIndex, uint32_t length) const
        {
            auto begin = Data();
            auto end = Data();
            UTF8::Advance(begin, beginIndex);
            UTF8::Advance(end, beginIndex + length);
            return StringSlice(begin, static_cast<uint32_t>(end - begin));
        }

        // O(1)
        [[nodiscard]] TChar ByteAt(uint32_t index) const
        {
            assert(index < Size());
            return Data()[index];
        }

        // O(N)
        [[nodiscard]] TCodepoint CodePointAt(uint32_t index) const
        {
            return Str::CodepointAt(Data(), Size(), index);
        }

        [[nodiscard]] Iterator FindFirstOf(Iterator start, TCodepoint search) const noexcept
        {
            const uint32_t size = Size();
            const TChar* data = Data();
            FE_CORE_ASSERT(start.m_Iter >= data && start.m_Iter <= data + size, "");

            const uint32_t searchSize = static_cast<uint32_t>(data + size - start.m_Iter);
            return Str::FindFirstOf(start.m_Iter, searchSize, search);
        }

        [[nodiscard]] Iterator FindFirstOf(TCodepoint search) const noexcept
        {
            return Str::FindFirstOf(Data(), Size(), search);
        }

        [[nodiscard]] Iterator FindLastOf(TCodepoint search) const noexcept
        {
            return Str::FindLastOf(Data(), Size(), search);
        }

        [[nodiscard]] bool Contains(TCodepoint search) const noexcept
        {
            return FindFirstOf(search) != end();
        }

        [[nodiscard]] bool StartsWith(StringSlice prefix, bool caseSensitive = true) const noexcept
        {
            if (prefix.Size() > Size())
                return false;

            return UTF8::AreEqual(Data(), prefix.Data(), prefix.Size(), prefix.Size(), caseSensitive);
        }

        [[nodiscard]] bool EndsWith(StringSlice suffix, bool caseSensitive = true) const noexcept
        {
            if (suffix.Size() > Size())
                return false;

            return UTF8::AreEqual(Data() + Size() - suffix.Size(), suffix.Data(), suffix.Size(), suffix.Size(), caseSensitive);
        }

        template<class TVector>
        void Split(TVector& vector, uint32_t maxCount, TCodepoint c = ' ') const
        {
            if (maxCount == 0)
                return;

            auto current = begin();
            while (current != end())
            {
                auto cPos = maxCount-- == 0 ? end() : FindFirstOf(current, c);
                vector.emplace_back(current.m_Iter, cPos.m_Iter - current.m_Iter);
                current = cPos;
                if (current != end())
                    ++current;
            }
        }

        [[nodiscard]] festd::pmr::vector<StringSlice> Split(TCodepoint c = ' ',
                                                            std::pmr::memory_resource* pAllocator = nullptr) const
        {
            if (pAllocator == nullptr)
                pAllocator = std::pmr::get_default_resource();

            festd::pmr::vector<StringSlice> result{ pAllocator };
            Split(result, UINT32_MAX, c);
            return result;
        }

        [[nodiscard]] festd::pmr::vector<StringSlice> SplitLines(std::pmr::memory_resource* pAllocator = nullptr) const
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

        [[nodiscard]] StringSlice StripLeft(StringSlice chars = "\n\r\t ") const noexcept
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

        [[nodiscard]] StringSlice StripRight(StringSlice chars = "\n\r\t ") const noexcept
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

        [[nodiscard]] StringSlice Strip(StringSlice chars = "\n\r\t ") const noexcept
        {
            return StripLeft(chars).StripRight(chars);
        }

        [[nodiscard]] int Compare(const StringSlice& other) const noexcept
        {
            return UTF8::Compare(Data(), other.Data(), Size(), other.Size());
        }

        [[nodiscard]] bool IsEqualTo(const StringSlice& other, bool caseSensitive = true) const noexcept
        {
            return UTF8::AreEqual(Data(), other.Data(), Size(), other.Size(), caseSensitive);
        }

        template<class T>
        [[nodiscard]] festd::expected<T, ParseError> Parse() const
        {
            T result;
            auto err = ParseImpl(result);
            if (err == ParseErrorCode::None)
                return result;

            return festd::unexpected(err);
        }

        [[nodiscard]] explicit operator UUID() const noexcept
        {
            return UUID(Data());
        }

        [[nodiscard]] explicit operator Env::Name() const noexcept
        {
            return Env::Name{ Data(), Size() };
        }

        [[nodiscard]] Iterator begin() const noexcept
        {
            return Iterator(Data());
        }

        [[nodiscard]] Iterator end() const noexcept
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
        static ParseError TryConvert(const StringSlice& str, UUID& result)
        {
            result = UUID::Parse({ str.Data(), str.Size() });
            return result.IsZero() ? ParseErrorCode::InvalidSyntax : ParseErrorCode::None;
        }
    };
} // namespace FE
