#pragma once
#include <FeCore/Base/Base.h>
#include <cassert>
#include <cstdint>
#include <string>

namespace FE::UTF8
{
    using TChar            = char;
    using TCharTraits      = std::char_traits<TChar>;
    using TCodepoint       = char32_t;
    using TCodepointTraits = std::char_traits<TCodepoint>;

    // assertion without loggers, used in modules on which loggers depend
#define FE_CORE_ASSERT(expression, msg)                                                                                          \
    do                                                                                                                           \
    {                                                                                                                            \
        if (!(expression))                                                                                                       \
        {                                                                                                                        \
            FE_DEBUGBREAK;                                                                                                       \
            (void)msg;                                                                                                           \
        }                                                                                                                        \
    }                                                                                                                            \
    while (0)

    namespace Internal
    {
        /*
         * We use branchless UTF-8 decoder https://github.com/skeeto/branchless-utf8.
         */

        /* Decode the next character, C, from BUF, reporting errors in E.
         *
         * Since this is a branchless decoder, four bytes will be read from the
         * buffer regardless of the actual length of the next character. This
         * means the buffer _must_ have at least three bytes of zero padding
         * following the end of the data stream.
         *
         * Errors are reported in E, which will be non-zero if the parsed
         * character was somehow invalid: invalid byte sequence, non-canonical
         * encoding, or a surrogate half.
         *
         * The function returns a pointer to the next character. When an error
         * occurs, this pointer will be a guess that depends on the particular
         * error, but it will always advance at least one byte.
         */
        static void* utf8_decode(void* buf, UInt32* c, int* e) noexcept
        {
            static const char lengths[]  = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                            0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0 };
            static const int masks[]     = { 0x00, 0x7f, 0x1f, 0x0f, 0x07 };
            static const UInt32 mins[] = { 4194304, 0, 128, 2048, 65536 };
            static const int shiftc[]    = { 0, 18, 12, 6, 0 };
            static const int shifte[]    = { 0, 6, 4, 2, 0 };

            unsigned char* s = (unsigned char*)buf;
            int len          = lengths[s[0] >> 3];

            /* Compute the pointer to the next character early so that the next
             * iteration can start working on the next character. Neither Clang
             * nor GCC figure out this reordering on their own.
             */
            unsigned char* next = s + len + !len;

            /* Assume a four-byte character and load four bytes. Unused bits are
             * shifted out.
             */
            *c = (UInt32)(s[0] & masks[len]) << 18;
            *c |= (UInt32)(s[1] & 0x3f) << 12;
            *c |= (UInt32)(s[2] & 0x3f) << 6;
            *c |= (UInt32)(s[3] & 0x3f) << 0;
            *c >>= shiftc[len];

            /* Accumulate the various error conditions. */
            *e = (*c < mins[len]) << 6;      // non-canonical encoding
            *e |= ((*c >> 11) == 0x1b) << 7; // surrogate half?
            *e |= (*c > 0x10FFFF) << 8;      // out of range?
            *e |= (s[1] & 0xc0) >> 2;
            *e |= (s[2] & 0xc0) >> 4;
            *e |= (s[3]) >> 6;
            *e ^= 0x2a; // top two bits of each tail byte correct?
            *e >>= shifte[len];

            return next;
        }
    } // namespace Internal

    inline TCodepoint Decode(const TChar*& it) noexcept
    {
        UInt32 c;
        int e;
        void* input = reinterpret_cast<void*>(const_cast<TChar*>(it));
        it          = static_cast<const TChar*>(Internal::utf8_decode(input, &c, &e));
        FE_CORE_ASSERT(e == 0, "Invalid unicode");
        return c;
    }

    inline TCodepoint DecodePrior(const TChar*& it) noexcept
    {
        UInt32 c;
        int e             = 1;
        const TChar* iter = it;
        int maxlen        = 3;
        while (e && maxlen--)
        {
            void* input                 = reinterpret_cast<void*>(const_cast<TChar*>(--iter));
            [[maybe_unused]] void* temp = Internal::utf8_decode(input, &c, &e);
        }

        FE_CORE_ASSERT(e == 0, "Invalid prior unicode");
        it = iter;
        return c;
    }

    inline TCodepoint PeekDecode(const TChar* it) noexcept
    {
        return Decode(it);
    }

    inline int Compare(const TChar* lhs, const TChar* rhs, size_t length) noexcept
    {
        for (; 0 < length; --length, Decode(lhs), Decode(rhs))
        {
            if (*lhs != *rhs)
            {
                return PeekDecode(lhs) < PeekDecode(rhs) ? -1 : +1;
            }
        }

        return 0;
    }

    inline size_t Length(const TChar* str, size_t byteLen) noexcept
    {
        size_t result = 0;
        while (byteLen-- && Decode(str))
            ++result;
        return result;
    }

    inline void Advance(const TChar*& str, size_t n) noexcept
    {
        while (n-- && Decode(str)) {}
    }

    inline bool Valid(const TChar* str) noexcept
    {
        UInt32 c;
        int e = 0;
        while (*str)
        {
            void* input = reinterpret_cast<void*>(const_cast<TChar*>(str));
            str         = static_cast<const TChar*>(Internal::utf8_decode(input, &c, &e));
        }
        return e == 0;
    }
} // namespace FE::UTF8
