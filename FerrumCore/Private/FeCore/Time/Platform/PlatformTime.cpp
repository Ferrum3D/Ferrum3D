#include <FeCore/Time/DateTime.h>

namespace FE::Platform
{
    inline static uint32_t IsYearLeap(int64_t year)
    {
        return (!(year % 4) && ((year % 100) || !(year % 400))) ? 1 : 0;
    }


    inline static int64_t GetDayCountInYear(int64_t year)
    {
        return IsYearLeap(year) ? 366 : 365;
    }


    inline constexpr int64_t YearTable[2][12] = { { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
                                                  { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };


    inline constexpr int64_t EpochYear = 1970;
    inline constexpr int64_t SecondsInDay = 24 * 60 * 60;


    SystemTimeInfo DeconstructTime(TimeValue time)
    {
        const TimeValue dayClock = time % SecondsInDay;
        TimeValue srcDay = time / SecondsInDay;

        SystemTimeInfo result;
        result.m_second = dayClock % 60;
        result.m_minute = static_cast<int8_t>((dayClock % 3600) / 60);
        result.m_hour = static_cast<int8_t>(dayClock / 3600);
        result.m_dayOfWeek = (srcDay + 4) % 7;

        int64_t year = 1970;
        while (srcDay >= GetDayCountInYear(year))
        {
            srcDay -= GetDayCountInYear(year);
            year++;
        }

        result.m_year = static_cast<int16_t>(year - 1900);
        result.m_month = 0;
        while (srcDay >= YearTable[IsYearLeap(year)][result.m_month])
        {
            srcDay -= YearTable[IsYearLeap(year)][result.m_month];
            result.m_month++;
        }

        result.m_day = static_cast<int8_t>(srcDay + 1);
        return result;
    }


    TimeValue ConstructTime(SystemTimeInfo dateTime)
    {
        const int64_t srcYear = static_cast<int64_t>(dateTime.m_year) + 1900;

        int64_t srcDay = (srcYear - EpochYear) * 365;
        srcDay += (srcYear - EpochYear) / 4 + ((srcYear % 4) && srcYear % 4 < EpochYear % 4);
        srcDay -= (srcYear - EpochYear) / 100 + ((srcYear % 100) && srcYear % 100 < EpochYear % 100);
        srcDay += (srcYear - EpochYear) / 400 + ((srcYear % 400) && srcYear % 400 < EpochYear % 400);

        int64_t yearDay = 0;
        int64_t month = 0;
        while (month < dateTime.m_month)
        {
            yearDay += YearTable[IsYearLeap(srcYear)][month];
            month++;
        }

        yearDay += static_cast<int64_t>(dateTime.m_day) - 1;
        srcDay += yearDay;

        TimeValue result = srcDay * SecondsInDay;
        result += (static_cast<int64_t>(dateTime.m_hour) * 60 + dateTime.m_minute) * 60;
        result += dateTime.m_second;
        return result;
    }
} // namespace FE::Platform
