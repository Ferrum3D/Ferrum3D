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
        inline int32_t Years() const
        {
            return m_Data.tm_year;
        }

        inline int32_t Days() const
        {
            return m_Data.tm_mday;
        }

        inline int32_t Hours() const
        {
            return m_Data.tm_hour;
        }

        inline int32_t Minutes() const
        {
            return m_Data.tm_min;
        }

        inline int32_t Seconds() const
        {
            auto s = m_Data.tm_sec;
            if (s == 60)
                return 0;
            return s;
        }

        inline int32_t TotalSeconds() const
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
