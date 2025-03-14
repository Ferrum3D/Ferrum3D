#pragma once
#include <FeCore/Base/Base.h>

namespace FE
{
    //! @brief A universally unique identifier.
    struct UUID final
    {
        enum class FormatFlags : uint32_t
        {
            //! @brief Compact UUID form: hexadecimal digits only, e.g. `0123456789abcdef0123456789abcdef`
            kCompact = 0,

            //! @brief Adds hyphens, e.g. `01234567-89ab-cdef-0123-56789abcdef`
            kHyphens = 1 << 0,

            //! @brief Adds braces, e.g. `{0123456789abcdef0123456789abcdef}`
            kBraces = 1 << 1,

            //! @brief Uppercase form, e.g. `0123456789ABCDEF0123456789ABCDEF`
            kUppercase = 1 << 2,

            //! @brief Adds hyphens and braces, e.g. `{01234567-89ab-cdef-0123-56789abcdef}`
            kHyphensAndBraces = kHyphens | kBraces,

            //! @brief Default format, with hyphens and braces, e.g. `01234567-89AB-CDEF-0123-56789ABCDEF`
            kDefault = kHyphens | kUppercase,
        };

        UUID() noexcept
            : m_simdVector(_mm_setzero_si128())
        {
        }

        explicit UUID(const __m128i simdVector)
            : m_simdVector(simdVector)
        {
        }

        //! @brief Parse a UUID from a string in form `"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"`.
        explicit UUID(const festd::ascii_view string) noexcept
        {
            m_simdVector = Parse(string).m_simdVector;
        }

        [[nodiscard]] uint8_t* data() noexcept
        {
            return m_bytes;
        }

        [[nodiscard]] const uint8_t* data() const noexcept
        {
            return m_bytes;
        }

        [[nodiscard]] size_t size() const noexcept
        {
            return sizeof(m_simdVector);
        }

        [[nodiscard]] bool IsZero() const noexcept
        {
            return _mm_movemask_epi8(_mm_cmpeq_epi8(m_simdVector, _mm_setzero_si128())) == 0xffff;
        }

        [[nodiscard]] bool IsValid() const noexcept
        {
            return !IsZero();
        }

        FE_FORCE_INLINE static UUID FE_VECTORCALL Invalid()
        {
            return UUID{};
        }

        static UUID FE_VECTORCALL Parse(festd::ascii_view string);

        template<FormatFlags TFormat>
        static UUID FE_VECTORCALL Parse(festd::ascii_view string);

        FE_FORCE_INLINE static int32_t FE_VECTORCALL Compare(const UUID lhs, const UUID rhs)
        {
            const __m128i kShuffle = _mm_setr_epi8(3, 2, 1, 0, 5, 4, 7, 6, 9, 8, 15, 14, 13, 12, 11, 10);
            const __m128i kRotateSigned = _mm_set1_epi8(static_cast<int8_t>(128));

            const __m128i x = _mm_add_epi8(_mm_shuffle_epi8(lhs.m_simdVector, kShuffle), kRotateSigned);
            const __m128i y = _mm_add_epi8(_mm_shuffle_epi8(rhs.m_simdVector, kShuffle), kRotateSigned);

            const __m128i lessMask = _mm_cmplt_epi8(x, y);
            const __m128i greaterMask = _mm_cmpgt_epi8(x, y);

            return _mm_movemask_epi8(greaterMask) - _mm_movemask_epi8(lessMask);
        }

        union
        {
            __m128i m_simdVector;
            uint8_t m_bytes[16];
        };
    };

    FE_ENUM_OPERATORS(UUID::FormatFlags);


    FE_FORCE_INLINE UUID FE_VECTORCALL UUID::Parse(const festd::ascii_view string)
    {
        switch (string.length())
        {
        case 32:
            return Parse<FormatFlags::kCompact>(string);
        case 34:
            return Parse<FormatFlags::kBraces>(string);
        case 36:
            return Parse<FormatFlags::kHyphens>(string);
        case 38:
            return Parse<(FormatFlags::kHyphens | FormatFlags::kBraces)>(string);
        default:
            return Invalid();
        }
    }


    template<UUID::FormatFlags TFormat>
    FE_FORCE_INLINE UUID FE_VECTORCALL UUID::Parse(festd::ascii_view string)
    {
        if (Bit::AllSet(TFormat, FormatFlags::kBraces | FormatFlags::kHyphens))
        {
            FE_Assert(string.length() == 38 && string[0] == '{' && string[37] == '}', "Invalid format");
            string = string.substr(1, 36);
        }
        else if (Bit::AnySet(TFormat, FormatFlags::kBraces))
        {
            FE_Assert(string.length() == 34 && string[0] == '{' && string[33] == '}', "Invalid format");
            string = string.substr(1, 32);
        }

        __m128i mask1, mask2;
        if (Bit::AnySet(TFormat, FormatFlags::kHyphens))
        {
            const __m128i lower = _mm_loadu_si128(reinterpret_cast<const __m128i*>(string.data()));
            const __m128i upper = _mm_loadu_si128(reinterpret_cast<const __m128i*>(string.data() + 3) + 1);

            __m128i lower1 = _mm_shuffle_epi8(lower, _mm_setr_epi8(0, 2, 4, 6, 9, 11, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1));
            __m128i lower2 = _mm_shuffle_epi8(lower, _mm_setr_epi8(1, 3, 5, 7, 10, 12, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1));
            __m128i upper1 = _mm_shuffle_epi8(upper, _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 5, 7, 9, 11, 13, -1));
            __m128i upper2 = _mm_shuffle_epi8(upper, _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 1, 3, 6, 8, 10, 12, 14, -1));

            lower1 = _mm_insert_epi8(lower1, string[16], 7);
            lower2 = _mm_insert_epi8(lower2, string[17], 7);
            upper1 = _mm_insert_epi8(upper1, string[34], 15);
            upper2 = _mm_insert_epi8(upper2, string[35], 15);

            mask1 = _mm_or_si128(lower1, upper1);
            mask2 = _mm_or_si128(lower2, upper2);
        }
        else
        {
            const __m128i lower = _mm_loadu_si128(reinterpret_cast<const __m128i*>(string.data()));
            const __m128i upper = _mm_loadu_si128(reinterpret_cast<const __m128i*>(string.data()) + 1);

            __m128i lower1 = _mm_shuffle_epi8(lower, _mm_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, -1, -1, -1, -1, -1, -1, -1, -1));
            __m128i lower2 = _mm_shuffle_epi8(lower, _mm_setr_epi8(1, 3, 5, 7, 9, 11, 13, 15, -1, -1, -1, -1, -1, -1, -1, -1));
            __m128i upper1 = _mm_shuffle_epi8(upper, _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 0, 2, 4, 6, 8, 10, 12, 14));
            __m128i upper2 = _mm_shuffle_epi8(upper, _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 1, 3, 5, 7, 9, 11, 13, 15));

            mask1 = _mm_or_si128(lower1, upper1);
            mask2 = _mm_or_si128(lower2, upper2);
        }

        const int32_t kCompareFlags = _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_NEGATIVE_POLARITY;
        const __m128i kAllowedSymbols = _mm_setr_epi8('0', '9', 'A', 'F', 'a', 'f', 0, -1, 0, -1, 0, -1, 0, -1, 0, -1);
        if (_mm_cmpistri(kAllowedSymbols, mask1, kCompareFlags) != sizeof(__m128i)
            || _mm_cmpistri(kAllowedSymbols, mask2, kCompareFlags) != sizeof(__m128i))
        {
            return Invalid();
        }

        const __m128i nine = _mm_set1_epi8('9');
        const __m128i aboveNine1 = _mm_cmpgt_epi8(mask1, nine);
        const __m128i aboveNine2 = _mm_cmpgt_epi8(mask2, nine);

        const __m128i lowercaseMask = _mm_set1_epi8(0x20);
        const __m128i letters1 = _mm_or_si128(_mm_and_si128(mask1, aboveNine1), lowercaseMask);
        const __m128i letters2 = _mm_or_si128(_mm_and_si128(mask2, aboveNine2), lowercaseMask);

        const __m128i hex = _mm_set1_epi8('a' - 10 - '0');
        const __m128i upperBlend = _mm_blendv_epi8(mask1, _mm_sub_epi8(letters1, hex), aboveNine1);
        const __m128i lowerBlend = _mm_blendv_epi8(mask2, _mm_sub_epi8(letters2, hex), aboveNine2);

        const __m128i zero = _mm_set1_epi8('0');
        const __m128i lowerResult = _mm_sub_epi8(lowerBlend, zero);
        const __m128i upperResult = _mm_slli_epi16(_mm_sub_epi8(upperBlend, zero), 4);

        return UUID{ _mm_xor_si128(lowerResult, upperResult) };
    }


    FE_FORCE_INLINE bool FE_VECTORCALL operator<(const UUID lhs, const UUID rhs)
    {
        return UUID::Compare(lhs, rhs) < 0;
    }


    FE_FORCE_INLINE bool FE_VECTORCALL operator==(const UUID lhs, const UUID rhs)
    {
        const __m128i mask = _mm_cmpeq_epi8(lhs.m_simdVector, rhs.m_simdVector);
        return _mm_movemask_epi8(mask) == 0xffff;
    }


    FE_FORCE_INLINE bool FE_VECTORCALL operator!=(const UUID lhs, const UUID rhs)
    {
        return !(lhs == rhs);
    }
} // namespace FE

template<>
struct eastl::hash<FE::UUID>
{
    size_t operator()(const FE::UUID value) const noexcept
    {
        return FE::DefaultHash(value.data(), sizeof(value.m_simdVector));
    }
};
