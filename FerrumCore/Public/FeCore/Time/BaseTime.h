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
        int32_t m_year : 16;     //!< Year since 1900.
        int32_t m_month : 8;     //!< Month 0-11.
        int32_t m_day : 8;       //!< Day of month 1-31.
        int32_t m_dayOfWeek : 8; //!< Day since Sunday 0-6.
        int32_t m_hour : 8;      //!< Hour 0-23.
        int32_t m_minute : 8;    //!< Minutes 0-59.
        int32_t m_second : 8;    //!< Seconds 0-60.
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


        //! @brief Equivalent to QueryPerformanceCounter() on Windows.
        [[nodiscard]] uint64_t GetTicks();


        //! @brief Get ticks per second to pair with GetTicks().
        [[nodiscard]] double GetTicksPerSecond();


        //! @brief Get seconds per tick to pair with GetTicks().
        [[nodiscard]] double GetSecondsPerTick();


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


    struct HighResolutionTimer final
    {
        HighResolutionTimer() = default;

        void Start()
        {
            m_isRunning = true;
            m_isValid = false;
            m_startTicks = Platform::GetTicks();
        }

        void Stop()
        {
            m_endTicks = Platform::GetTicks();
            FE_Assert(m_isRunning);
            m_isRunning = false;
            m_isValid = true;
            m_endTicks = Math::Max(m_endTicks, m_startTicks);
        }

        [[nodiscard]] uint64_t GetElapsedTicks() const
        {
            FE_Assert(m_isValid);
            return m_endTicks - m_startTicks;
        }

        [[nodiscard]] double GetElapsedSeconds() const
        {
            FE_Assert(m_isValid);
            return static_cast<double>(m_endTicks - m_startTicks) * Platform::GetSecondsPerTick();
        }

        [[nodiscard]] double GetElapsedMilliseconds() const
        {
            return GetElapsedSeconds() * 1'000.0;
        }

        [[nodiscard]] double GetElapsedMicroseconds() const
        {
            return GetElapsedSeconds() * 1'000'000.0;
        }

        [[nodiscard]] double GetElapsedNanoseconds() const
        {
            return GetElapsedSeconds() * 1'000'000'000.0;
        }

    private:
        uint64_t m_startTicks = 0;
        uint64_t m_endTicks = 0;
        bool m_isRunning = false;
        bool m_isValid = false;
    };
} // namespace FE
