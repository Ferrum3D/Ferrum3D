#pragma once
#include <FeCore/Strings/Encoding.h>

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
            FE_Assert(bytesRead > 0, "Unicode Error");
            return result;
        }

        StrIterator& operator++()
        {
            value_type result;
            const int32_t bytesRead = UTF8::DecodeForward(m_iter, -1, &result);
            FE_Assert(bytesRead > 0, "Unicode Error");
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
            FE_Assert(bytesRead > 0, "Unicode Error");
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
