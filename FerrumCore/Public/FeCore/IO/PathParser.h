#pragma once
#include <festd/Internal/StringBase.h>

namespace FE::PathParser
{
    [[nodiscard]] FE_FORCE_INLINE constexpr bool IsPathSeparator(const int32_t codepoint)
    {
        return codepoint == '/' || codepoint == '\\';
    }


    [[nodiscard]] FE_FORCE_INLINE constexpr bool HasWindowsDrivePrefix(const char* str, const uint32_t length)
    {
        return length >= 2 && ASCII::IsLetter(str[0]) && str[1] == ':';
    }


    [[nodiscard]] FE_FORCE_INLINE constexpr bool HasWindowsUNCPrefix(const char* str, const uint32_t length)
    {
        return length >= 3 && IsPathSeparator(str[0]) && IsPathSeparator(str[1]) && !IsPathSeparator(str[2]);
    }


    [[nodiscard]] constexpr const char* SkipPathRoot(const char* str, const uint32_t length)
    {
        if (length < 2)
            return str;

        if (HasWindowsDrivePrefix(str, length))
            return str + 2;

        if (HasWindowsUNCPrefix(str, length))
        {
            const char* prefixEnd = str + 2;
            while (prefixEnd < str + length && !IsPathSeparator(*prefixEnd))
                ++prefixEnd;

            return prefixEnd;
        }

        return str;
    }


    [[nodiscard]] constexpr const char* SkipSeparators(const char* str, const uint32_t length)
    {
        const char* end = str + length;
        while (str < end && IsPathSeparator(*str))
            ++str;

        return str;
    }


    [[nodiscard]] constexpr const char* SkipUntilSeparator(const char* str, const uint32_t length)
    {
        const char* end = str + length;
        while (str < end && !IsPathSeparator(*str))
            ++str;

        return str;
    }


    [[nodiscard]] constexpr bool IsAbsolutePath(const char* str, const uint32_t length)
    {
        return length > 0 && str[0] == '/' || SkipPathRoot(str, length) != str;
    }


    //! @brief Compare two UTF8-encoded strings that represent file system paths.
    //!
    //! This functions works just like UTF8::Compare, except it considers '/' and '\' the same.
    [[nodiscard]] inline int32_t ComparePaths(const char* lhs, const char* rhs, const uint32_t length1,
                                              const uint32_t length2) noexcept
    {
        uint32_t index1 = 0;
        uint32_t index2 = 0;
        while (index1 < length1 && index2 < length2)
        {
            int32_t lcp, rcp;
            index1 += UTF8::DecodeForward(lhs + index1, length1 - index1 + 1, &lcp);
            index2 += UTF8::DecodeForward(rhs + index2, length2 - index2 + 1, &rcp);

            if (lcp != rcp && (!IsPathSeparator(lcp) || !IsPathSeparator(rcp)))
                return lcp < rcp ? -1 : 1;
        }

        if (length1 == length2)
            return 0;

        return length1 < length2 ? -1 : 1;
    }
} // namespace FE::PathParser
