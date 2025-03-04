#pragma once
#include <FeCore/Base/Base.h>
#include <utf8proc.h>

namespace FE::Str
{
    [[nodiscard]] FE_FORCE_INLINE char* Copy(char* dst, const uint32_t dstSize, const char* src, const uint32_t srcSize)
    {
        const uint32_t actualSize = dstSize < srcSize ? dstSize : srcSize;
        memcpy(dst, src, actualSize);
        return dst + actualSize;
    }
} // namespace FE::Str


namespace FE::ASCII
{
    [[nodiscard]] FE_FORCE_INLINE constexpr uint32_t Length(const char* str)
    {
        return static_cast<uint32_t>(__builtin_strlen(str));
    }


    [[nodiscard]] FE_FORCE_INLINE int32_t Compare(const char* lhs, const char* rhs) noexcept
    {
        return strcmp(lhs, rhs);
    }


    [[nodiscard]] FE_FORCE_INLINE int32_t Compare(const char* lhs, const char* rhs, const uint32_t count) noexcept
    {
        return strncmp(lhs, rhs, count);
    }


    [[nodiscard]] FE_FORCE_INLINE constexpr bool IsUpper(const char c)
    {
        return static_cast<uint32_t>(c) - 'A' < 26;
    }


    [[nodiscard]] FE_FORCE_INLINE constexpr bool IsLower(const char c)
    {
        return static_cast<uint32_t>(c) - 'a' < 26;
    }


    [[nodiscard]] FE_FORCE_INLINE constexpr bool ToUpper(const char c)
    {
        return static_cast<uint32_t>(c) - 'a' < 26 ? c & 0x5f : c;
    }


    [[nodiscard]] FE_FORCE_INLINE constexpr bool ToLower(const char c)
    {
        return static_cast<uint32_t>(c) - 'A' ? c | 0x20 : c;
    }


    [[nodiscard]] FE_FORCE_INLINE constexpr bool IsDigit(const char c)
    {
        return static_cast<uint32_t>(c) - '0' < 10;
    }


    [[nodiscard]] FE_FORCE_INLINE constexpr bool IsLetter(const char c)
    {
        return static_cast<uint32_t>(c | 0x20) - 'a' < 26;
    }


    [[nodiscard]] FE_FORCE_INLINE constexpr bool IsValid(const int32_t codepoint)
    {
        return !(codepoint & ~0x7f);
    }
} // namespace FE::ASCII

namespace FE::UTF8
{
    [[nodiscard]] FE_FORCE_INLINE int32_t DecodeForward(const char* str, const int32_t byteSize, int32_t* result)
    {
        return static_cast<int32_t>(utf8proc_iterate(reinterpret_cast<const utf8proc_uint8_t*>(str), byteSize, result));
    }


    [[nodiscard]] FE_FORCE_INLINE int32_t DecodeBackward(const char* str, int32_t* result)
    {
        for (int32_t byteIndex = -1; byteIndex >= -4; --byteIndex)
        {
            const int32_t bytesRead = DecodeForward(&str[byteIndex], -1, result);
            if (bytesRead > 0)
                return -byteIndex;
        }

        return -1;
    }


    [[nodiscard]] FE_FORCE_INLINE int32_t Encode(const int32_t codepoint, char* destination)
    {
        const utf8proc_ssize_t result = utf8proc_encode_char(codepoint, reinterpret_cast<utf8proc_uint8_t*>(destination));
        return static_cast<int32_t>(result);
    }


    [[nodiscard]] inline uint32_t Length(const char* str, const int32_t byteSize)
    {
        if (str == nullptr)
            return 0;

        uint32_t length = 0;
        while (*str)
        {
            int32_t codepoint;
            const int32_t bytesRead = DecodeForward(str, byteSize, &codepoint);
            if (bytesRead < 0)
                return length;

            str += bytesRead;
            ++length;
        }

        return length;
    }


    [[nodiscard]] inline int32_t Compare(const char* lhs, const char* rhs, const uint32_t length1,
                                         const uint32_t length2) noexcept
    {
        uint32_t index1 = 0;
        uint32_t index2 = 0;
        while (index1 < length1 && index2 < length2)
        {
            int32_t lcp, rcp;
            index1 += DecodeForward(lhs + index1, length1 - index1 + 1, &lcp);
            index2 += DecodeForward(rhs + index2, length2 - index2 + 1, &rcp);
            if (lcp != rcp)
                return lcp < rcp ? -1 : 1;
        }

        if (length1 == length2)
            return 0;

        return length1 < length2 ? -1 : 1;
    }


    [[nodiscard]] FE_FORCE_INLINE bool IsUpper(const int32_t codepoint)
    {
        return utf8proc_isupper(codepoint);
    }


    [[nodiscard]] FE_FORCE_INLINE bool IsLower(const int32_t codepoint)
    {
        return utf8proc_islower(codepoint);
    }


    [[nodiscard]] FE_FORCE_INLINE bool ToUpper(const int32_t codepoint)
    {
        return utf8proc_toupper(codepoint);
    }


    [[nodiscard]] FE_FORCE_INLINE bool ToLower(const int32_t codepoint)
    {
        return utf8proc_tolower(codepoint);
    }


    [[nodiscard]] FE_FORCE_INLINE bool IsLetter(const int32_t codepoint)
    {
        const utf8proc_category_t category = utf8proc_category(codepoint);
        switch (category)
        {
        case UTF8PROC_CATEGORY_LU:
        case UTF8PROC_CATEGORY_LL:
        case UTF8PROC_CATEGORY_LT:
        case UTF8PROC_CATEGORY_LM:
        case UTF8PROC_CATEGORY_LO:
            return true;
        default:
            return false;
        }
    }


    [[nodiscard]] FE_FORCE_INLINE bool IsSpace(const int32_t codepoint)
    {
        const utf8proc_category_t category = utf8proc_category(codepoint);
        switch (category)
        {
        case UTF8PROC_CATEGORY_ZS:
        case UTF8PROC_CATEGORY_ZL:
        case UTF8PROC_CATEGORY_ZP:
        case UTF8PROC_CATEGORY_CC:
        case UTF8PROC_CATEGORY_CF:
        case UTF8PROC_CATEGORY_CS:
            return true;
        default:
            return false;
        }
    }


    [[nodiscard]] inline bool IsValid(const char* str)
    {
        if (str == nullptr)
            return true;

        while (*str)
        {
            int32_t codepoint;
            const int32_t bytesRead = DecodeForward(str, -1, &codepoint);
            if (bytesRead < 0)
                return false;

            str += bytesRead;
        }

        return true;
    }
} // namespace FE::UTF8

namespace FE::Internal
{
    struct StrIterator final
    {
        const char* m_iter;

        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = int32_t;
        using pointer = const int32_t*;
        using reference = int32_t;

        value_type operator*() const
        {
            value_type result;
            const int32_t bytesRead = UTF8::DecodeForward(m_iter, -1, &result);
            FE_CORE_ASSERT(bytesRead > 0, "Unicode Error");
            return result;
        }

        StrIterator& operator++()
        {
            value_type result;
            const int32_t bytesRead = UTF8::DecodeForward(m_iter, -1, &result);
            FE_CORE_ASSERT(bytesRead > 0, "Unicode Error");
            m_iter += bytesRead;
            return *this;
        }

        StrIterator operator++(int)
        {
            StrIterator t = *this;
            ++(*this);
            return t;
        }

        StrIterator& operator--()
        {
            value_type result;
            const int32_t bytesRead = UTF8::DecodeBackward(m_iter, &result);
            FE_CORE_ASSERT(bytesRead > 0, "Unicode Error");
            m_iter -= bytesRead;
            return *this;
        }

        StrIterator operator--(int)
        {
            StrIterator t = *this;
            --(*this);
            return t;
        }

        friend StrIterator operator+(StrIterator lhs, int32_t rhs)
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

        friend StrIterator operator-(const StrIterator& lhs, const int32_t rhs)
        {
            return lhs + (-rhs);
        }

        friend bool operator==(const StrIterator& a, const StrIterator& b)
        {
            return a.m_iter == b.m_iter;
        }

        friend bool operator!=(const StrIterator& a, const StrIterator& b)
        {
            return a.m_iter != b.m_iter;
        }
    };
} // namespace FE::Internal
