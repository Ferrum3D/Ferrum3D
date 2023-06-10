#include <FeCore/Strings/StringSlice.h>

namespace FE
{
    ParseError StringSlice::TryToUIntImpl(UInt64& result) const
    {
        if (Size() == 0)
        {
            return ParseErrorCode::UnexpectedEnd;
        }

        const TChar* ptr    = Data();
        const TChar* endPtr = Data() + Size();

        result = 0;
        while (true)
        {
            if (ptr == endPtr)
            {
                return ParseErrorCode::None;
            }

            if (*ptr <= '9' && *ptr >= '0')
            {
                result *= 10;
                result += *ptr - '0';
            }
            else
            {
                return { ParseErrorCode::InvalidSyntax, static_cast<USize>(ptr - Data()) };
            }

            ++ptr;
        }
    }

    ParseError StringSlice::TryToIntImpl(Int64& result) const
    {
        if (Size() == 0)
        {
            return ParseErrorCode::UnexpectedEnd;
        }

        const TChar* ptr    = Data();
        const TChar* endPtr = Data() + Size();
        auto isNegative     = false;

        if (*ptr == '-')
        {
            isNegative = true;
            ++ptr;
        }

        if (endPtr - ptr == 0)
        {
            return { ParseErrorCode::UnexpectedEnd, 1 };
        }

        result = 0;
        while (true)
        {
            if (ptr == endPtr)
            {
                if (isNegative)
                {
                    result = -result;
                }

                return ParseErrorCode::None;
            }

            if (*ptr <= '9' && *ptr >= '0')
            {
                result *= 10;
                result += *ptr - '0';
            }
            else
            {
                return { ParseErrorCode::InvalidSyntax, static_cast<USize>(ptr - Data()) };
            }

            ++ptr;
        }
    }

    ParseError StringSlice::TryToFloatImpl(Float64& result) const
    {
        if (Size() < 1)
        {
            return ParseErrorCode::UnexpectedEnd;
        }

        auto foundDot = false;

        auto slice = *this;
        if (*slice.Data() == '-')
        {
            slice = StringSlice(slice.Data() + 1, slice.Size() - 1);
        }

        USize position = 0;
        for (auto c : slice)
        {
            if (c == '.')
            {
                if (foundDot)
                {
                    return { ParseErrorCode::InvalidSyntax, position };
                }

                foundDot = true;
            }
            else if (c > '9' || c < '0')
            {
                return { ParseErrorCode::InvalidSyntax, position };
                continue;
            }

            ++position;
        }

        result = std::strtod(Data(), nullptr);
        return ParseErrorCode::None;
    }
} // namespace FE
