#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Strings/Unicode.h>

namespace FE
{
    using UTF8::TChar;
    using UTF8::TCharTraits;
    using UTF8::TCodepoint;
    using UTF8::TCodepointTraits;

    class Str;
    class String;
    class StringSlice;

    template<size_t TCapacity>
    class FixedString;

    namespace Internal
    {
        class StrIterator final
        {
            friend class FE::Str;
            friend class FE::String;
            friend class FE::StringSlice;

            template<size_t TCapacity>
            friend class FE::FixedString;

            const TChar* m_Iter;

        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = TCodepoint;
            using pointer = const TCodepoint*;
            using reference = const TCodepoint&;

            StrIterator(const TChar* iter)
                : m_Iter(iter)
            {
            }

            value_type operator*() const
            {
                return UTF8::PeekDecode(m_Iter);
            }

            StrIterator& operator++()
            {
                UTF8::Decode(m_Iter);
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
                UTF8::DecodePrior(m_Iter);
                return *this;
            }

            StrIterator operator--(int)
            {
                StrIterator t = *this;
                UTF8::DecodePrior(m_Iter);
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

            friend StrIterator operator-(const StrIterator& lhs, int32_t rhs)
            {
                return lhs + (-rhs);
            }

            friend bool operator==(const StrIterator& a, const StrIterator& b)
            {
                return a.m_Iter == b.m_Iter;
            }

            friend bool operator!=(const StrIterator& a, const StrIterator& b)
            {
                return a.m_Iter != b.m_Iter;
            }
        };
    } // namespace Internal


    class Str final
    {
        using Iter = Internal::StrIterator;

    public:
        [[nodiscard]] constexpr static uint32_t ByteLength(const TChar* str) noexcept
        {
            return static_cast<uint32_t>(__builtin_strlen(str));
        }

        [[nodiscard]] static uint32_t Length(const TChar* str, uint32_t byteSize) noexcept
        {
            return UTF8::Length(str, byteSize);
        }

        [[nodiscard]] static uint32_t Length(const TChar* str) noexcept
        {
            return UTF8::Length(str, ByteLength(str));
        }

        [[nodiscard]] static TChar* Copy(TChar* dst, uint32_t dstSize, const TChar* src, uint32_t srcSize)
        {
            const uint32_t actualSize = dstSize < srcSize ? dstSize : srcSize;
            memcpy(dst, src, actualSize);
            return dst + actualSize;
        }

        [[nodiscard]] static TChar* Copy(std::pmr::memory_resource* allocator, const TChar* src, uint32_t srcSize)
        {
            void* dst = allocator->allocate(srcSize);
            memcpy(dst, src, srcSize);
            return static_cast<TChar*>(dst);
        }

        [[nodiscard]] static int32_t ByteCompare(const TChar* lhs, const TChar* rhs) noexcept
        {
            return strcmp(lhs, rhs);
        }

        [[nodiscard]] static int32_t ByteCompare(const TChar* lhs, const TChar* rhs, uint32_t count) noexcept
        {
            return strncmp(lhs, rhs, count);
        }

        [[nodiscard]] static int32_t Compare(const TChar* lhs, const TChar* rhs, uint32_t length1, uint32_t length2) noexcept
        {
            return UTF8::Compare(lhs, rhs, length1, length2);
        }

        [[nodiscard]] static int32_t Compare(const TChar* lhs, const TChar* rhs) noexcept
        {
            return Compare(lhs, rhs, ByteLength(lhs), ByteLength(rhs));
        }

        [[nodiscard]] static TCodepoint CodepointAt(const TChar* str, uint32_t size, uint32_t index)
        {
            const TChar* begin = str;
            const TChar* end = begin + size + 1;
            uint32_t cpIndex = 0;
            for (auto iter = begin; iter != end; UTF8::Decode(iter), ++cpIndex)
            {
                if (cpIndex == index)
                    return UTF8::PeekDecode(iter);
            }

            FE_CORE_ASSERT(0, "Out of range");
            return 0;
        }

        [[nodiscard]] static const TChar* FindFirstOf(const TChar* str, uint32_t size, TCodepoint search) noexcept
        {
            const Iter e{ str + size };
            for (Iter iter = str; iter != e; ++iter)
            {
                if (*iter == search)
                {
                    return iter.m_Iter;
                }
            }

            return e.m_Iter;
        }

        [[nodiscard]] static const TChar* FindLastOf(const TChar* str, uint32_t size, TCodepoint search) noexcept
        {
            const Iter e{ str + size };
            Iter result = e;
            for (Iter iter = str; iter != e; ++iter)
            {
                if (*iter == search)
                {
                    result = iter;
                }
            }

            return result.m_Iter;
        }

        [[nodiscard]] static bool Contains(const TChar* str, uint32_t size, TCodepoint search) noexcept
        {
            return FindFirstOf(str, size, search) != str + size;
        }

        [[nodiscard]] static const TChar* StripLeft(const TChar* str, uint32_t size, const TChar* chars,
                                                    uint32_t charsSize) noexcept
        {
            const Iter endIter{ str + size };
            Iter result{ str };
            for (Iter iter = str; iter != endIter; ++iter)
            {
                if (!Contains(chars, charsSize, *iter))
                {
                    break;
                }

                result = iter;
                ++result;
            }

            return result.m_Iter;
        }

        [[nodiscard]] static const TChar* StripRight(const TChar* str, uint32_t size, const TChar* chars,
                                                     uint32_t charsSize) noexcept
        {
            const Iter beginIter{ str };
            Iter result{ str + size };
            Iter iter = result;
            --iter;
            for (; iter != beginIter; --iter)
            {
                if (!Contains(chars, charsSize, *iter))
                {
                    break;
                }

                result = iter;
            }

            return result.m_Iter;
        }
    };
} // namespace FE
