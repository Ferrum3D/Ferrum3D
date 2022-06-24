#pragma once
#include <FeCore/Strings/Format.h>
#include <FeCore/Time/TimeSpan.h>

namespace FE
{
    //! \brief Represents date and time.
    class DateTime
    {
        tm m_Data;

        inline DateTime(const tm& data)
            : m_Data(data)
        {
        }

        inline time_t MakeTime() const
        {
            auto copy = m_Data;
            return mktime(&copy);
        }

    public:
        FE_CLASS_RTTI(DateTime, "9927EE14-F009-464A-A067-4E4CB7AFE56C");

        inline Int32 Year() const
        {
            return m_Data.tm_year + 1900;
        }

        inline Int32 Day() const
        {
            return m_Data.tm_mday;
        }

        inline Int32 DayOfWeek() const
        {
            return m_Data.tm_wday;
        }

        inline Int32 DayOfYear() const
        {
            return m_Data.tm_yday + 1;
        }

        inline Int32 Hour() const
        {
            return m_Data.tm_hour;
        }

        inline Int32 Minute() const
        {
            return m_Data.tm_min;
        }

        inline Int32 Second() const
        {
            auto s = m_Data.tm_sec;
            if (s == 60)
                return 0;
            return s;
        }

        void Format(std::ostream& stream, const char* format = "[%m/%d/%Y %T]") const;

        String ToString(const char* format = "[%m/%d/%Y %T]") const;

        inline static DateTime CreateLocal(time_t time)
        {
            tm data;
            localtime_s(&data, &time);
            return DateTime(data);
        }

        inline static DateTime CreateUtc(time_t time)
        {
            tm data;
            gmtime_s(&data, &time);
            return DateTime(data);
        }

        inline static DateTime Now()
        {
            time_t now;
            time(&now);
            return CreateLocal(now);
        }

        inline static DateTime UtcNow()
        {
            time_t now;
            time(&now);
            return CreateUtc(now);
        }

        inline TimeSpan operator-(const DateTime& other) const
        {
            time_t diff = static_cast<time_t>(difftime(MakeTime(), other.MakeTime()));
            return TimeSpan::FromSeconds(diff);
        }
    };

    inline std::ostream& operator<<(std::ostream& stream, const DateTime& time)
    {
        time.Format(stream);
        return stream;
    }
} // namespace FE
