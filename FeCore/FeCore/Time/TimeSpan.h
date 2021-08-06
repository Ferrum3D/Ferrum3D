#pragma once
#include <FeCore/Utils/CoreUtils.h>
#include <FeCore/Utils/Platform.h>
#include <ctime>
#include <ostream>
#include <sstream>

namespace FE
{
    class FE_CORE_API TimeSpan
    {
        tm m_Data;

        inline TimeSpan(time_t time)
        {
            localtime_s(&m_Data, &time);
        }

    public:
        inline Int32 Years() const
        {
            return m_Data.tm_year;
        }

        inline Int32 Days() const
        {
            return m_Data.tm_mday;
        }

        inline Int32 Hours() const
        {
            return m_Data.tm_hour;
        }

        inline Int32 Minutes() const
        {
            return m_Data.tm_min;
        }

        inline Int32 Seconds() const
        {
            auto s = m_Data.tm_sec;
            if (s == 60)
                return 0;
            return s;
        }

        inline Int32 TotalSeconds() const
        {
            auto copy = m_Data;
            return mktime(&copy);
        }

        inline static TimeSpan FromSeconds(time_t seconds)
        {
            return TimeSpan(seconds);
        }
    };
} // namespace FE
