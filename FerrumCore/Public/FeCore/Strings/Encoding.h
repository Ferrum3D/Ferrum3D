#pragma once
#include <FeCore/Base/Base.h>
#include <utf8proc.h>

namespace FE::Str
{
    //! @brief Copy a string to a buffer.
    //!
    //! This function copies max(destinationSize, sourceSize) bytes from the source string to the destination buffer.
    //!
    //! @param destination     The buffer to copy the string to.
    //! @param destinationSize The size of the destination buffer in bytes.
    //! @param source          The string to copy.
    //! @param sourceSize      The size of the source string in bytes.
    //!
    //! @return A pointer to the end of the copied string in the destination buffer.
    [[nodiscard]] FE_FORCE_INLINE char* Copy(char* destination, const uint32_t destinationSize, const char* source,
                                             const uint32_t sourceSize)
    {
        const uint32_t actualSize = destinationSize < sourceSize ? destinationSize : sourceSize;
        memcpy(destination, source, actualSize);
        return destination + actualSize;
    }
} // namespace FE::Str


namespace FE::ASCII
{
    //! @brief Get the length of a null-terminated ASCII string.
    [[nodiscard]] FE_FORCE_INLINE constexpr uint32_t Length(const char* str)
    {
        return static_cast<uint32_t>(__builtin_strlen(str));
    }


    //! @brief Compare two null-terminated ASCII strings.
    [[nodiscard]] FE_FORCE_INLINE int32_t Compare(const char* lhs, const char* rhs) noexcept
    {
        return strcmp(lhs, rhs);
    }


    //! @brief Compare two null-terminated ASCII strings reading at most count characters.
    [[nodiscard]] FE_FORCE_INLINE int32_t Compare(const char* lhs, const char* rhs, const uint32_t count) noexcept
    {
        return strncmp(lhs, rhs, count);
    }


    //! @brief Check if a character is an uppercase letter.
    [[nodiscard]] FE_FORCE_INLINE constexpr bool IsUpper(const char c)
    {
        return static_cast<uint32_t>(c) - 'A' < 26;
    }


    //! @brief Check if a character is a lowercase letter.
    [[nodiscard]] FE_FORCE_INLINE constexpr bool IsLower(const char c)
    {
        return static_cast<uint32_t>(c) - 'a' < 26;
    }


    //! @brief Convert a character to uppercase.
    [[nodiscard]] FE_FORCE_INLINE constexpr bool ToUpper(const char c)
    {
        return static_cast<uint32_t>(c) - 'a' < 26 ? c & 0x5f : c;
    }


    //! @brief Convert a character to lowercase.
    [[nodiscard]] FE_FORCE_INLINE constexpr bool ToLower(const char c)
    {
        return static_cast<uint32_t>(c) - 'A' ? c | 0x20 : c;
    }


    //! @brief Check if a character is a digit.
    [[nodiscard]] FE_FORCE_INLINE constexpr bool IsDigit(const char c)
    {
        return static_cast<uint32_t>(c) - '0' < 10;
    }


    //! @brief Check if a character is a letter.
    [[nodiscard]] FE_FORCE_INLINE constexpr bool IsLetter(const char c)
    {
        return static_cast<uint32_t>(c | 0x20) - 'a' < 26;
    }


    //! @brief Check if the provided codepoint represents a valid ASCII character.
    [[nodiscard]] FE_FORCE_INLINE constexpr bool IsValid(const int32_t codepoint)
    {
        return !(codepoint & ~0x7f);
    }
} // namespace FE::ASCII


namespace FE::UTF8
{
    //! @brief Decode a UTF-8 codepoint from a string.
    //!
    //! @param str      The string to read the codepoint from.
    //! @param byteSize The number of bytes to read from the string.
    //! @param result   The decoded codepoint.
    //!
    //! @return The number of bytes read from the string or -1 if the function failed.
    [[nodiscard]] FE_FORCE_INLINE int32_t DecodeForward(const char* str, const int32_t byteSize, int32_t* result)
    {
        return static_cast<int32_t>(utf8proc_iterate(reinterpret_cast<const utf8proc_uint8_t*>(str), byteSize, result));
    }


    //! @brief Decode a UTF-8 codepoint from a string reading backwards.
    //!
    //! @param str    The string to read the codepoint from.
    //! @param result The decoded codepoint.
    //!
    //! @return The negative number of bytes read from the string or 0 if the function failed.
    [[nodiscard]] FE_FORCE_INLINE int32_t DecodeBackward(const char* str, int32_t* result)
    {
        for (int32_t byteIndex = -1; byteIndex >= -4; --byteIndex)
        {
            const int32_t bytesRead = DecodeForward(&str[byteIndex], -1, result);
            if (bytesRead > 0)
                return -byteIndex;
        }

        return 0;
    }


    //! @brief Encode a UTF-8 codepoint into a string.
    //!
    //! @param codepoint   The codepoint to encode.
    //! @param destination The string to write the codepoint into.
    //!
    //! @return The number of bytes written to the string or 0 if the function failed.
    [[nodiscard]] FE_FORCE_INLINE int32_t Encode(const int32_t codepoint, char* destination)
    {
        const utf8proc_ssize_t result = utf8proc_encode_char(codepoint, reinterpret_cast<utf8proc_uint8_t*>(destination));
        return static_cast<int32_t>(result);
    }


    //! @brief Get the length of a null-terminated UTF-8 string as a number of codepoints.
    //!
    //! @param str      The string to get the length of.
    //! @param byteSize The maximum number of bytes to read from the string.
    //!
    //! @return The length of the string as a number of codepoints.
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


    //! @brief Get the length of a null-terminated UTF-8 string in bytes.
    [[nodiscard]] inline uint32_t ByteLength(const char* str)
    {
        return static_cast<uint32_t>(__builtin_strlen(str));
    }


    //! @brief Compare two null-terminated UTF-8 strings.
    //!
    //! @param lhs     The first string to compare.
    //! @param rhs     The second string to compare.
    //! @param length1 The maximum number of bytes to read from the first string.
    //! @param length2 The maximum number of bytes to read from the second string.
    //!
    //! @return -1 if lhs is less than rhs, 0 if lhs is equal to rhs, and 1 if lhs is greater than rhs.
    [[nodiscard]] inline int32_t Compare(const char* lhs, const char* rhs, const uint32_t length1,
                                         const uint32_t length2) noexcept
    {
        uint32_t index1 = 0;
        uint32_t index2 = 0;
        while (index1 < length1 && index2 < length2)
        {
            int32_t lcp, rcp;
            index1 += DecodeForward(lhs + index1, static_cast<int32_t>(length1 - index1 + 1), &lcp);
            index2 += DecodeForward(rhs + index2, static_cast<int32_t>(length2 - index2 + 1), &rcp);
            if (lcp != rcp)
                return lcp < rcp ? -1 : 1;
        }

        if (length1 == length2)
            return 0;

        return length1 < length2 ? -1 : 1;
    }


    //! @brief Check if a codepoint is an uppercase letter.
    [[nodiscard]] FE_FORCE_INLINE bool IsUpper(const int32_t codepoint)
    {
        return utf8proc_isupper(codepoint);
    }


    //! @brief Check if a codepoint is a lowercase letter.
    [[nodiscard]] FE_FORCE_INLINE bool IsLower(const int32_t codepoint)
    {
        return utf8proc_islower(codepoint);
    }


    //! @brief Convert a codepoint to uppercase.
    [[nodiscard]] FE_FORCE_INLINE bool ToUpper(const int32_t codepoint)
    {
        return utf8proc_toupper(codepoint);
    }


    //! @brief Convert a codepoint to lowercase.
    [[nodiscard]] FE_FORCE_INLINE bool ToLower(const int32_t codepoint)
    {
        return utf8proc_tolower(codepoint);
    }


    //! @brief Check if a codepoint is a letter.
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


    //! @brief Check if a codepoint is a space.
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


    //! @brief Check if a null-terminated string is valid UTF-8.
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


namespace FE::UTF16
{
    //! @brief Decode a UTF-16 codepoint from a string.
    //!
    //! @param str      The string to read the codepoint from.
    //! @param byteSize The number of bytes to read from the string.
    //! @param result   The decoded codepoint.
    //!
    //! @return The number of bytes read from the string or -1 if the function failed.
    [[nodiscard]] FE_FORCE_INLINE int32_t DecodeForward(const char16_t* str, const int32_t byteSize, int32_t* result)
    {
        constexpr char16_t kHighSurrogateStart = 0xd800;
        constexpr char16_t kLowSurrogateStart = 0xdc00;
        constexpr char16_t kHighSurrogateEnd = 0xdbff;
        constexpr char16_t kLowSurrogateEnd = 0xdfff;

        // Use unsigned comparison, so that -1 is always considered enough
        if (static_cast<uint32_t>(byteSize) == 0)
            return 0;

        if (str[0] >= kHighSurrogateStart && str[0] <= kHighSurrogateEnd)
        {
            if (static_cast<uint32_t>(byteSize) <= 1)
                return -1;

            if (str[1] < kLowSurrogateStart || str[1] > kLowSurrogateEnd)
                return -1;

            *result = 0x10000 + ((str[0] & 0x3ff) << 10) + (str[1] & 0x3ff);
            return 2;
        }

        if (str[0] >= kLowSurrogateStart && str[0] <= kLowSurrogateEnd)
            return -1;

        *result = str[0];
        return 1;
    }


    //! @brief Decode a UTF-16 codepoint from a string reading backwards.
    //!
    //! @param str    The string to read the codepoint from.
    //! @param result The decoded codepoint.
    //!
    //! @return The negative number of bytes read from the string or 0 if the function failed.
    [[nodiscard]] FE_FORCE_INLINE int32_t DecodeBackward(const char16_t* str, int32_t* result)
    {
        for (int32_t charIndex = -1; charIndex >= -2; --charIndex)
        {
            const int32_t charsRead = DecodeForward(&str[charIndex], -1, result);
            if (charsRead > 0)
                return -charIndex;
        }

        return 0;
    }


    //! @brief Encode a UTF-16 codepoint into a string.
    //!
    //! @param codepoint   The codepoint to encode.
    //! @param destination The string to write the codepoint into.
    //!
    //! @return The number of bytes written to the string or 0 if the function failed.
    [[nodiscard]] FE_FORCE_INLINE int32_t Encode(const int32_t codepoint, char16_t* destination)
    {
        if (codepoint <= 0xffff)
        {
            destination[0] = static_cast<char16_t>(codepoint);
            return 1;
        }

        destination[0] = static_cast<char16_t>(0xd800 + ((codepoint - 0x10000) >> 10));
        destination[1] = static_cast<char16_t>(0xdc00 + (codepoint & 0x3ff));
        return 2;
    }


    //! @brief Get the length of a null-terminated UTF-16 string as a number of codepoints.
    //!
    //! @param str  The string to get the length of.
    //! @param size The maximum number of characters to read from the string.
    //!
    //! @return The length of the string as a number of codepoints.
    [[nodiscard]] inline uint32_t Length(const char16_t* str, const int32_t size)
    {
        if (str == nullptr)
            return 0;

        uint32_t length = 0;
        while (*str)
        {
            int32_t codepoint;
            const int32_t charsRead = DecodeForward(str, size, &codepoint);
            if (charsRead < 0)
                return length;

            str += charsRead;
            ++length;
        }

        return length;
    }


    [[nodiscard]] inline uint32_t ByteLength(const char16_t* str)
    {
        if (str == nullptr)
            return 0;

        uint32_t byteCount = 0;
        while (*str)
        {
            byteCount += sizeof(char16_t);
            ++str;
        }

        return byteCount;
    }
} // namespace FE::UTF16


namespace FE::Str
{
    namespace Internal
    {
        template<class TDestinationChar, uint32_t TSize>
        struct ConversionHelperBase
        {
            ConversionHelperBase() = default;

            ConversionHelperBase(const ConversionHelperBase&) = delete;
            ConversionHelperBase(ConversionHelperBase&&) = delete;
            ConversionHelperBase& operator=(const ConversionHelperBase&) = delete;
            ConversionHelperBase& operator=(ConversionHelperBase&&) = delete;

            ~ConversionHelperBase()
            {
                if (m_capacity > TSize)
                    m_allocator->deallocate(m_data, m_capacity);
                m_capacity = 0;
                m_data = nullptr;
            }

            void Initialize(const uint32_t capacity)
            {
                if (m_capacity > TSize)
                    m_allocator->deallocate(m_data, m_capacity);

                if (capacity == 0)
                {
                    m_data = nullptr;
                    m_capacity = 0;
                    return;
                }

                if (capacity <= TSize)
                {
                    m_data = m_inlineBuffer;
                    m_capacity = capacity;
                    return;
                }

                m_data = static_cast<TDestinationChar*>(m_allocator->allocate(capacity));
                m_capacity = capacity;
            }

            std::pmr::memory_resource* m_allocator = nullptr;
            TDestinationChar m_inlineBuffer[TSize];
            TDestinationChar* m_data = nullptr;
            uint32_t m_capacity = 0;
            uint32_t m_length = 0;
        };
    } // namespace Internal


    //! @brief Convert a UTF-8 encoded string to a UTF-16 encoded string
    //!
    //! @param source          The UTF-8 encoded string to convert.
    //! @param sourceSize      The size of the UTF-8 encoded string in bytes or kMaxU32 to indicate a null-terminated string.
    //! @param destination     The buffer to write the UTF-16 encoded string to, must be large enough.
    //! @param destinationSize The size of the destination buffer in characters.
    //!
    //! @return The number of characters written to the destination buffer or kMaxU32 if the conversion failed.
    [[nodiscard]] uint32_t ConvertUtf8ToUtf16(const char* source, uint32_t sourceSize, char16_t* destination,
                                              uint32_t destinationSize);


    //! @brief Convert a UTF-16 encoded string to a UTF-8 encoded string
    //!
    //! @param source          The UTF-16 encoded string to convert.
    //! @param sourceSize      The size of the UTF-16 encoded string in characters or kMaxU32 to indicate a null-terminated string.
    //! @param destination     The buffer to write the UTF-8 encoded string to, must be large enough.
    //! @param destinationSize The size of the destination buffer in bytes.
    //!
    //! @return The number of bytes written to the destination buffer or kMaxU32 if the conversion failed.
    [[nodiscard]] uint32_t ConvertUtf16ToUtf8(const char16_t* source, uint32_t sourceSize, char* destination,
                                              uint32_t destinationSize);


    struct Utf8ToUtf16 final : private Internal::ConversionHelperBase<char16_t, 128>
    {
        explicit Utf8ToUtf16(const char* source, const uint32_t sourceSize, std::pmr::memory_resource* allocator = nullptr)
        {
            m_allocator = allocator ? allocator : std::pmr::get_default_resource();
            Convert(source, sourceSize);
        }

        explicit Utf8ToUtf16(const char* source, std::pmr::memory_resource* allocator = nullptr)
        {
            m_allocator = allocator ? allocator : std::pmr::get_default_resource();
            Convert(source, ASCII::Length(source));
        }

        [[nodiscard]] uint32_t size() const
        {
            return m_length - 1;
        }

        [[nodiscard]] const char16_t* data() const
        {
            return m_data;
        }

        [[nodiscard]] const wchar_t* ToWideString() const
        {
            static_assert(sizeof(char16_t) == sizeof(wchar_t));
            return reinterpret_cast<const wchar_t*>(m_data);
        }

    private:
        void Convert(const char* source, const uint32_t sourceSize)
        {
            Initialize(sourceSize + 1);

            const uint32_t result = ConvertUtf8ToUtf16(source, sourceSize, m_data, m_capacity);
            FE_Assert(result != Constants::kMaxU32, "Failed to convert UTF-8 to UTF-16");

            m_length = result;
        }
    };


    struct Utf16ToUtf8 final : private Internal::ConversionHelperBase<char, 256>
    {
        explicit Utf16ToUtf8(const char16_t* source, const uint32_t sourceSize, std::pmr::memory_resource* allocator = nullptr)
        {
            m_allocator = allocator ? allocator : std::pmr::get_default_resource();
            Convert(source, sourceSize);
        }

        explicit Utf16ToUtf8(const char16_t* source, std::pmr::memory_resource* allocator = nullptr)
        {
            m_allocator = allocator ? allocator : std::pmr::get_default_resource();
            Convert(source, UTF16::ByteLength(source) >> 1);
        }

        explicit Utf16ToUtf8(const wchar_t* source, const uint32_t sourceSize, std::pmr::memory_resource* allocator = nullptr)
        {
            static_assert(sizeof(char16_t) == sizeof(wchar_t));
            m_allocator = allocator ? allocator : std::pmr::get_default_resource();
            Convert(reinterpret_cast<const char16_t*>(source), sourceSize);
        }

        explicit Utf16ToUtf8(const wchar_t* source, std::pmr::memory_resource* allocator = nullptr)
        {
            static_assert(sizeof(char16_t) == sizeof(wchar_t));
            m_allocator = allocator ? allocator : std::pmr::get_default_resource();
            Convert(reinterpret_cast<const char16_t*>(source), UTF16::ByteLength(reinterpret_cast<const char16_t*>(source)) >> 1);
        }

        [[nodiscard]] uint32_t size() const
        {
            return m_length - 1;
        }

        [[nodiscard]] const char* data() const
        {
            return m_data;
        }

    private:
        void Convert(const char16_t* source, const uint32_t sourceSize)
        {
            Initialize(sourceSize + 1);

            const uint32_t result = ConvertUtf16ToUtf8(source, sourceSize, m_data, m_capacity);
            FE_Assert(result != Constants::kMaxU32, "Failed to convert UTF-16 to UTF-8");

            m_length = result;
        }
    };
} // namespace FE::Str
