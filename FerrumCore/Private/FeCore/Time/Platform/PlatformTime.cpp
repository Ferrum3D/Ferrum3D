#include <FeCore/Time/DateTime.h>

namespace FE::Platform
{
    namespace
    {
        uint32_t IsYearLeap(const int64_t year)
        {
            return (!(year % 4) && ((year % 100) || !(year % 400))) ? 1 : 0;
        }


        int64_t GetDayCountInYear(const int64_t year)
        {
            return IsYearLeap(year) ? 366 : 365;
        }


        constexpr int64_t kYearTable[2][12] = { { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
                                                { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };


        constexpr int64_t kEpochYear = 1970;
        constexpr int64_t kSecondsInDay = 24 * 60 * 60;
    } // namespace


    SystemTimeInfo DeconstructTime(const TimeValue time)
    {
        const TimeValue dayClock = time % kSecondsInDay;
        TimeValue srcDay = time / kSecondsInDay;

        SystemTimeInfo result;
        result.m_second = static_cast<int32_t>(dayClock % 60);
        result.m_minute = static_cast<int32_t>((dayClock % 3600) / 60);
        result.m_hour = static_cast<int32_t>(dayClock / 3600);
        result.m_dayOfWeek = static_cast<int32_t>((srcDay + 4) % 7);

        int64_t year = 1970;
        while (srcDay >= GetDayCountInYear(year))
        {
            srcDay -= GetDayCountInYear(year);
            year++;
        }

        const uint32_t isLeap = IsYearLeap(year);

        result.m_year = static_cast<int32_t>(year - 1900);
        result.m_month = 0;
        while (srcDay >= kYearTable[isLeap][result.m_month])
        {
            srcDay -= kYearTable[isLeap][result.m_month];
            result.m_month++;
        }

        result.m_day = static_cast<int32_t>(srcDay + 1);
        return result;
    }


    TimeValue ConstructTime(const SystemTimeInfo dateTime)
    {
        const int64_t srcYear = static_cast<int64_t>(dateTime.m_year) + 1900;
        const uint32_t isLeap = IsYearLeap(srcYear);

        int64_t srcDay = (srcYear - kEpochYear) * 365;
        srcDay += (srcYear - kEpochYear) / 4 + ((srcYear % 4) && srcYear % 4 < kEpochYear % 4);
        srcDay -= (srcYear - kEpochYear) / 100 + ((srcYear % 100) && srcYear % 100 < kEpochYear % 100);
        srcDay += (srcYear - kEpochYear) / 400 + ((srcYear % 400) && srcYear % 400 < kEpochYear % 400);

        int64_t yearDay = 0;
        int64_t month = 0;
        while (month < dateTime.m_month)
        {
            yearDay += kYearTable[isLeap][month];
            month++;
        }

        yearDay += static_cast<int64_t>(dateTime.m_day) - 1;
        srcDay += yearDay;

        TimeValue result = srcDay * kSecondsInDay;
        result += (static_cast<int64_t>(dateTime.m_hour) * 60 + dateTime.m_minute) * 60;
        result += dateTime.m_second;
        return result;
    }
} // namespace FE::Platform
