#include <FeCore/Time/DateTime.h>

namespace FE::Internal
{
    namespace
    {
        constexpr festd::string_view kDayNames[] = {
            "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
        };


        constexpr festd::string_view kMonthNames[] = {
            "January", "February", "March",     "April",   "May",      "June",
            "July",    "August",   "September", "October", "November", "December",
        };


        char* StrToDecimal(char* str, uint32_t value, int32_t minLength)
        {
            char* begin = str;
            do
            {
                const uint64_t digit = value % 10;
                value /= 10;

                *str++ = static_cast<char>('0' + digit);
                --minLength;
            }
            while (value || minLength > 0);

            char* end = str;
            *str-- = 0;

            while (begin < str)
            {
                const char temp = *str;
                *str-- = *begin;
                *begin++ = temp;
            }

            return end;
        }
    } // namespace


    festd::basic_fixed_string<64> DateTimeBase::ToString(const DateTimeFormatKind formatKind) const
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
                             kDayNames[DayOfWeek()].data(),
                             kDayNames[DayOfWeek()].size());
            *iter++ = ',';
            *iter++ = ' ';
            iter = Str::Copy(iter,
                             static_cast<uint32_t>(sizeof(result) - (iter - result)),
                             kMonthNames[Month()].data(),
                             kMonthNames[Month()].size());
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
