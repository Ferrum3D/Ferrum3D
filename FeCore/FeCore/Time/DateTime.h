#pragma once
#include <Strings/Format.h>
#include "TimeSpan.h"

namespace FE
{
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
        inline int32_t Year() const
        {
            return m_Data.tm_year + 1900;
        }

        inline int32_t Day() const
        {
            return m_Data.tm_mday;
        }

        inline int32_t DayOfWeek() const
        {
            return m_Data.tm_wday;
        }

        inline int32_t DayOfYear() const
        {
            return m_Data.tm_yday + 1;
        }

        inline int32_t Hour() const
        {
            return m_Data.tm_hour;
        }

        inline int32_t Minute() const
        {
            return m_Data.tm_min;
        }

        inline int32_t Second() const
        {
            auto s = m_Data.tm_sec;
            if (s == 60)
                return 0;
            return s;
        }

        void Format(std::ostream& stream, const char* format = "[%m/%d/%Y %T]") const;

        String DateTime::ToString(const char* format = "[%m/%d/%Y %T]") const;

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
            time_t diff = difftime(MakeTime(), other.MakeTime());
            return TimeSpan::FromSeconds(diff);
        }
    };

    inline std::ostream& operator<<(std::ostream& stream, const DateTime& time)
    {
        time.Format(stream);
        return stream;
    }
} // namespace FE
