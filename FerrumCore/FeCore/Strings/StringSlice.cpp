#include <FeCore/Strings/StringSlice.h>
#include <cctype>

namespace FE
{
    bool StringSlice::TryToUIntImpl(UInt64& result) const
    {
        if (Size() == 0)
        {
            return false;
        }

        const TChar* ptr    = Data();
        const TChar* endPtr = Data() + Size();

        result = 0;
        while (true)
        {
            if (ptr == endPtr)
            {
                return true;
            }

            if (*ptr <= '9' && *ptr >= '0')
            {
                result *= 10;
                result += *ptr - '0';
            }
            else
            {
                return false;
            }

            ++ptr;
        }

        return false;
    }

    bool StringSlice::TryToIntImpl(Int64& result) const
    {
        if (Size() == 0)
        {
            return false;
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
            return false;
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
                return true;
            }

            if (*ptr <= '9' && *ptr >= '0')
            {
                result *= 10;
                result += *ptr - '0';
            }
            else
            {
                return false;
            }

            ++ptr;
        }

        return false;
    }

    bool StringSlice::TryToFloatImpl(Float64& result) const
    {
        if (Size() < 1)
        {
            return false;
        }

        auto foundDot = false;

        auto slice = *this;
        if (*slice.Data() == '-')
        {
            slice = StringSlice(slice.Data() + 1, slice.Size() - 1);
        }

        for (auto c : slice)
        {
            if (c == '.')
            {
                if (foundDot)
                {
                    return false;
                }

                foundDot = true;
                continue;
            }

            if (c <= '9' && c >= '0')
            {
                continue;
            }

            return false;
        }

        result = std::strtod(Data(), nullptr);
        return true;
    }
} // namespace FE
