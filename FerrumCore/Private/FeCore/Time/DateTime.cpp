#include <FeCore/Time/DateTime.h>

namespace FE::Internal
{
    inline constexpr StringSlice DayNames[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
    };


    inline constexpr StringSlice DayNamesShort[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
    };


    inline constexpr StringSlice MonthNames[] = {
        "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December",
    };


    inline constexpr StringSlice MonthNamesShort[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };


    inline static TChar* StrToDecimal(TChar* str, uint32_t value, int32_t minLength)
    {
        TChar* begin = str;
        do
        {
            const uint64_t digit = value % 10;
            value /= 10;

            *str++ = '0' + static_cast<TChar>(digit);
            --minLength;
        }
        while (value || minLength > 0);

        TChar* end = str;
        *str-- = 0;

        while (begin < str)
        {
            const TChar temp = *str;
            *str-- = *begin;
            *begin++ = temp;
        }

        return end;
    }


    FixStr64 DateTimeBase::ToString(DateTimeFormatKind formatKind) const
    {
        char result[64];
        result[0] = 0;

        char* iter = result;

        switch (formatKind)
        {
        default:
        case DateTimeFormatKind::kISO8601:
            iter = StrToDecimal(iter, Year(), 4);
            *iter++ = '-';
            iter = StrToDecimal(iter, Month() + 1, 2);
            *iter++ = '-';
            iter = StrToDecimal(iter, Day(), 2);
            *iter++ = 'T';
            iter = StrToDecimal(iter, Hour(), 2);
            *iter++ = ':';
            iter = StrToDecimal(iter, Minute(), 2);
            *iter++ = ':';
            iter = StrToDecimal(iter, Second(), 2);
            *iter++ = 'Z';
            break;
        case DateTimeFormatKind::kShort:
            iter = StrToDecimal(iter, Month() + 1, -1);
            *iter++ = '/';
            iter = StrToDecimal(iter, Day(), -1);
            *iter++ = '/';
            iter = StrToDecimal(iter, Year(), -1);
            break;
        case DateTimeFormatKind::kLong:
            iter = Str::Copy(iter,
                             static_cast<uint32_t>(sizeof(result) - (iter - result)),
                             DayNames[DayOfWeek()].Data(),
                             DayNames[DayOfWeek()].Size());
            *iter++ = ',';
            *iter++ = ' ';
            iter = Str::Copy(iter,
                             static_cast<uint32_t>(sizeof(result) - (iter - result)),
                             MonthNames[Month()].Data(),
                             MonthNames[Month()].Size());
            *iter++ = ' ';
            iter = StrToDecimal(iter, Day(), -1);
            *iter++ = ',';
            *iter++ = ' ';
            iter = StrToDecimal(iter, Year(), -1);
            break;
        }

        return { result, static_cast<uint32_t>(iter - result) };
    }
} // namespace FE::Internal
