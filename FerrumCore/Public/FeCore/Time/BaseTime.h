#pragma once
#include <festd/string.h>

namespace FE
{
    enum class DateTimeFormatKind
    {
        kISO8601,
        kShort,
        kLong,
    };


    using TimeValue = int64_t;


    //! @brief Decomposed date and time info.
    struct alignas(uint64_t) SystemTimeInfo final
    {
        int32_t Year : 16;     //!< Year since 1900.
        int32_t Month : 8;     //!< Month 0-11.
        int32_t Day : 8;       //!< Day of month 1-31.
        int32_t DayOfWeek : 8; //!< Day since Sunday 0-6.
        int32_t Hour : 8;      //!< Hour 0-23.
        int32_t Minute : 8;    //!< Minutes 0-59.
        int32_t Second : 8;    //!< Seconds 0-60.
    };

    static_assert(sizeof(SystemTimeInfo) == sizeof(uint64_t));


    namespace Platform
    {
        struct TimeZoneInfo final
        {
            //! @brief Difference in minutes between Universal Coordinated Time (UTC) and local time.
            //!
            //! UTC = Local + MinuteBias
            int32_t m_minuteBias = 0;

            //! @brief Standard time zone name, e.g. "EST".
            Env::Name m_standardName;
        };


        //! @brief Get system's local time zone info.
        [[nodiscard]] TimeZoneInfo GetTimeZoneInfo();


        //! @brief Get current system time in UTC format.
        [[nodiscard]] TimeValue GetCurrentTimeUTC();


        //! @brief Decompose TimeValue into SystemTimeInfo.
        //!
        //! @param time The time value to decompose.
        [[nodiscard]] SystemTimeInfo DeconstructTime(TimeValue time);


        //! @brief Create TimeValue from SystemTimeInfo.
        //!
        //! @param dateTime SystemTimeInfo to compose from.
        [[nodiscard]] TimeValue ConstructTime(SystemTimeInfo dateTime);


        //! @brief Convert date and time in UTC format to timezone specific local format.
        //!
        //! @param source SystemTimeInfo to convert from.
        //! @param result SystemTimeInfo to write the result to.
        //!
        //! @return True if the operation was successful.
        bool ConvertUTCToLocalTime(SystemTimeInfo source, SystemTimeInfo& result);


        //! @brief Convert timezone specific local date and time to UTC format.
        //!
        //! @param source SystemTimeInfo to convert from.
        //! @param result SystemTimeInfo to write the result to.
        //!
        //! @return True if the operation was successful.
        bool ConvertLocalTimeToUTC(SystemTimeInfo source, SystemTimeInfo& result);
    } // namespace Platform
} // namespace FE
